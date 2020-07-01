package de.tu_darmstadt.seemoo.nexmon.gui;

import android.app.DialogFragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;

public class ChannelSelectDialog extends DialogFragment {

    private ListView lvChannel;

    /**
     * Mandatory empty constructor for the fragment manager to instantiate the
     * fragment (e.g. upon screen orientation changes).
     */
    public ChannelSelectDialog() {
    }

    // TODO: Customize parameter initialization
    @SuppressWarnings("unused")
    public static ChannelSelectDialog newInstance() {
        ChannelSelectDialog fragment = new ChannelSelectDialog();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        if (container != null) {
            container.removeAllViews();
        }

        View view = inflater.inflate(R.layout.fragment_channel_list, container, false);

        lvChannel = (ListView) view.findViewById(R.id.lv_channel_selection);

        getDialog().setTitle("Select Channel");

        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        ArrayAdapter<String> adapter = new ArrayAdapter<>(getActivity(),
                android.R.layout.simple_list_item_1, android.R.id.text1, getResources().getStringArray(R.array.entries_list_channel_text));

        lvChannel.setAdapter(adapter);

        lvChannel.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String[] channels = getResources().getStringArray(R.array.entries_list_channel_value);
                String selectedChannel = channels[position];
                setWlanChannel(selectedChannel);

                dismiss();
            }
        });
    }

    private void setWlanChannel(String channel) {
        final Command command = new Command(0, "nexutil -i -s 30 -v " + channel);

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    RootTools.getShell(true).add(command);
                } catch(Exception e) {e.printStackTrace();}
            }
        }).start();
    }
}
