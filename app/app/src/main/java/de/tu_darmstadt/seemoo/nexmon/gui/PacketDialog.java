/*
 * Nexmon PenTestSuite
 * Copyright (C) 2016 Fabian Knapp
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package de.tu_darmstadt.seemoo.nexmon.gui;

import android.app.DialogFragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.google.gson.Gson;
import com.unnamed.b.atv.model.TreeNode;
import com.unnamed.b.atv.view.AndroidTreeView;

import java.util.ArrayList;
import java.util.Collections;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.sharky.ProtoNode;

public class PacketDialog extends DialogFragment {

    Packet packet;
    TreeNode root;

    public PacketDialog() {
    }

    public PacketDialog(Packet packet) {
        this.packet = packet;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putString("packet", new Gson().toJson(packet));
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);


    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.packet_dialog, container);
        LinearLayout llDialog = (LinearLayout) view.findViewById(R.id.ll_dialog);


        if (savedInstanceState != null) {
            if (savedInstanceState.containsKey("packet")) {
                packet = new Gson().fromJson(savedInstanceState.getString("packet"), Packet.class);
            }
        }

        getDialog().setTitle("Frame " + packet._number);

        root = TreeNode.root();
        generateTree();

        AndroidTreeView tview = new AndroidTreeView(getActivity(), root);

        tview.setDefaultContainerStyle(R.style.TreeNodeStyle, true);
        tview.setDefaultViewHolder(TreeNodeItemHolder.class);
        llDialog.addView(tview.getView());

        return view;
    }

    private void generateTree() {
        ArrayList<ProtoNode> list = packet.getParents();
        Collections.sort(list);

        for (ProtoNode node : list) {
            String result = "";
            for (String value : node.getValues()) {
                result += value + ", ";
            }
            result = result.trim().substring(0, result.length() - 2);
            if (result.equals("null"))
                result = "";

            String description = node.getDescription();
            if (!result.equals(""))
                description += ":";
            String complete = description + " " + result;

            TreeNode parentNode = new TreeNode(new TreeNodeItemHolder.TreeNodeItem(MyApplication.makeSectionOfTextBold(complete, description)));
            addChilds(parentNode, node);
            root.addChild(parentNode);
        }
    }

    private void addChilds(TreeNode treeNode, ProtoNode protoNode) {
        ArrayList<ProtoNode> list = packet.getChilds(protoNode.getKey());
        Collections.sort(list);

        for (ProtoNode node : list) {
            String result = "";

            for (String value : node.getValues()) {
                result += value + ", ";
            }
            result = result.trim().substring(0, result.length() - 2);
            if (result.equals("null"))
                result = "";

            String description = node.getDescription();
            if (!result.equals(""))
                description += ":";
            String complete = description + " " + result;


            TreeNode childNode = new TreeNode(new TreeNodeItemHolder.TreeNodeItem(MyApplication.makeSectionOfTextBold(complete, description)));
            addChilds(childNode, node);
            treeNode.addChild(childNode);
        }
    }


}
