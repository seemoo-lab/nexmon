package de.tu_darmstadt.seemoo.nexmon.gui;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;

/**
 * Created by fabian on 1/10/17.
 */

public class SurveyNotificationActivity extends Activity {

    @Override
    protected void onStart() {
        super.onStart();

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(MyApplication.getAppContext());
        prefs.edit().putBoolean("switch_survey_notification", false).commit();

        Uri webpage = Uri.parse("http://survey.seemoo.tu-darmstadt.de/limesurvey/index.php/465539?N00=" + MyApplication.getNexmonUID());

        Intent intent = new Intent(Intent.ACTION_VIEW, webpage);


        startActivity(intent);
        finish();
    }
}
