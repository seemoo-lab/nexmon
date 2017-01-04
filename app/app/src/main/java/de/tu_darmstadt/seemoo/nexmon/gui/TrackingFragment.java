package de.tu_darmstadt.seemoo.nexmon.gui;

import android.app.Fragment;
import android.os.Bundle;

import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;

/**
 * Created by fabian on 12/20/16.
 */

public abstract class TrackingFragment extends Fragment {

    protected Tracker mTracker;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Obtain the shared Tracker instance.
        mTracker = MyApplication.getDefaultTracker();
    }

    @Override
    public void onResume() {
        super.onResume();

        mTracker.setScreenName(getTrackingName());
        mTracker.send(new HitBuilders.ScreenViewBuilder().build());
    }

    public abstract String getTrackingName();
}
