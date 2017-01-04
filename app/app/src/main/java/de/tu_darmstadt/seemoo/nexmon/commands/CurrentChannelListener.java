package de.tu_darmstadt.seemoo.nexmon.commands;

/**
 * Created by fabian on 11/23/16.
 */

public interface CurrentChannelListener {

    void onCurrentChannelInfo(int channel);
    void onCurrentChannelError(String error);
}
