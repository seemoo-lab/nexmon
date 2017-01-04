package de.tu_darmstadt.seemoo.nexmon.stations;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class AttackInstanceReceiver extends BroadcastReceiver {


    @Override
    public void onReceive(Context context, Intent intent) {
        Attack.updateAmountOfInstances(intent);

        Attack.updateObserver();
    }
}
