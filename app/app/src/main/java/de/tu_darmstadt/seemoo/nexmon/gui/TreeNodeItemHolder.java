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

import android.content.Context;
import android.text.SpannableStringBuilder;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.github.johnkil.print.PrintView;
import com.unnamed.b.atv.model.TreeNode;

import de.tu_darmstadt.seemoo.nexmon.R;

public class TreeNodeItemHolder extends TreeNode.BaseNodeViewHolder<TreeNodeItemHolder.TreeNodeItem> {

    private PrintView arrowView;
    private TextView tvDescription;

    public TreeNodeItemHolder(Context context) {
        super(context);
    }


    @Override
    public View createNodeView(TreeNode node, TreeNodeItem treeNodeItem) {
        final LayoutInflater inflater = LayoutInflater.from(context);
        final View view = inflater.inflate(R.layout.packet_dialog_treeview, null, false);

        arrowView = (PrintView) view.findViewById(R.id.arrow_icon);
        tvDescription = (TextView) view.findViewById(R.id.tv_treenode_desc);
        tvDescription.setText(treeNodeItem.description);

        if (node.isLeaf())
            arrowView.setVisibility(View.INVISIBLE);

        return view;
    }

    @Override
    public void toggle(boolean active) {
        arrowView.setIconText(context.getResources().getString(active ? R.string.ic_keyboard_arrow_down : R.string.ic_keyboard_arrow_right));
    }

    ;


    public static class TreeNodeItem {
        public SpannableStringBuilder description;

        public TreeNodeItem(SpannableStringBuilder description) {
            this.description = description;

        }
    }
}
