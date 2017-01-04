package de.tu_darmstadt.seemoo.nexmon.commands;

import com.stericson.RootShell.execution.Command;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;

/**
 * Created by fabian on 11/23/16.
 */

public class CurrentChannelCommand extends Command {


    private CurrentChannelListener currentChannelListener;
    private boolean isChannelListenerValid = false;

    public CurrentChannelCommand(CurrentChannelListener currentChannelListener) {
        super(0, 0, "iwlist " + MyApplication.WLAN_INTERFACE + " channel | grep 'Current Frequency:'");
        this.currentChannelListener = currentChannelListener;
        isChannelListenerValid = true;
    }

    public void commandOutput(int id, String line) {
        String currentChannel = line;

        try {
            if (currentChannel != null && currentChannel.contains("(Channel")) {
                String channelString = currentChannel.trim();
                int start = channelString.indexOf("(Channel") + 8;
                int end = channelString.indexOf(")");
                int channel = Integer.valueOf(channelString.substring(start, end).trim());
                if(isChannelListenerValid)
                    currentChannelListener.onCurrentChannelInfo(channel);
            }
        } catch(Exception e) {
            e.printStackTrace();
            if(isChannelListenerValid)
                currentChannelListener.onCurrentChannelError(e.getMessage());
        }

        super.commandOutput(id, line);
    }

    public void removeListener() {
        isChannelListenerValid = false;
        currentChannelListener = null;
    }
}

