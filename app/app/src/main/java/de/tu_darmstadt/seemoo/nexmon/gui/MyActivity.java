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

import android.Manifest;
import android.app.Activity;
import android.app.Fragment;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.ActionBarDrawerToggle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.widget.DrawerLayout;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

import com.google.gson.Gson;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.net.FrameReceiver;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;

public class MyActivity extends Activity {


    final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();
    FrameReceiver frameReceiver;
    ActionBarDrawerToggle mDrawerToggle;
    DrawerLayout drawerLayout;
    ListView navList;
    ArrayList<String> menuItems;
    ArrayAdapter<String> adapter;

    @Nullable
    IPermissionListener permissionListener;

    Activity activity = this;
    Fragment sharkFragment = new SharkFragment();
    Fragment airodumpFragment = new APfragment();
    Fragment settingsFragment = new SettingsFragment();
    Fragment airdecapFragment = new AirdecapFragment();
    Fragment aircrackWepFragment = AircrackWepFragment.newInstance();
    Fragment attackInfoFragment;
    Fragment pcapConcatFragment = PcapConcatFragment.newInstance();
    Fragment ivsMergeFragment = IvsMergeFragment.newInstance();
    Fragment pcapToIvsFragment = PcapToIvsFragment.newInstance();
    Fragment wpaDictFragment = WpaDictFragment.newInstance();
    Fragment firmwareFragment = FirmwareFragment.newInstance();
    Fragment toolsFragment = ToolsFragment.newInstance();
    Fragment aboutFragment = AboutUsFragment.newInstance();
    Fragment startFragment = StartFragment.newInstance();

    // Storage Permissions
    public static final int REQUEST_EXTERNAL_STORAGE = 1;
    public static final String[] PERMISSIONS_STORAGE = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    /**
     * Checks if the app has permission to write to device storage
     *
     * If the app does not has permission then the user will be prompted to grant permissions
     *
     * @param activity
     */
    public static void verifyStoragePermissions(Activity activity) {
        // Check if we have write permission
        int permission = getStoragePermission(activity);

        if (permission != PackageManager.PERMISSION_GRANTED) {
            // We don't have permission so prompt the user
            ActivityCompat.requestPermissions(
                    activity,
                    PERMISSIONS_STORAGE,
                    REQUEST_EXTERNAL_STORAGE
            );
        }
    }

    public static int getStoragePermission(Activity activity) {
        return ActivityCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if(permissionListener != null) {
            try {
                permissionListener.onPermissionResult(requestCode,permissions, grantResults);
            } catch(Exception e) {e.printStackTrace();}
        }
    }

    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 3];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 3] = hexArray[v >>> 4];
            hexChars[j * 3 + 1] = hexArray[v & 0x0F];
            hexChars[j * 3 + 2] = ' ';


        }
        return new String(hexChars);

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);

        drawerLayout = (DrawerLayout) findViewById(R.id.drawerLayout);
        navList = (ListView) findViewById(R.id.navList);

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);



        if (savedInstanceState != null && getStoragePermission(this) == PackageManager.PERMISSION_GRANTED) {
            if (savedInstanceState.containsKey("sharkFragment"))
                sharkFragment = getFragmentManager().getFragment(savedInstanceState, "sharkFragment");
            if (savedInstanceState.containsKey("airodumpFragment"))
                airodumpFragment = getFragmentManager().getFragment(savedInstanceState, "airodumpFragment");
            if (savedInstanceState.containsKey("settingsFragment"))
                settingsFragment = getFragmentManager().getFragment(savedInstanceState, "settingsFragment");
            if (savedInstanceState.containsKey("airdecapFragment"))
                airdecapFragment = getFragmentManager().getFragment(savedInstanceState, "airdecapFragment");
            if (savedInstanceState.containsKey("aircrackWepFragment"))
                aircrackWepFragment = getFragmentManager().getFragment(savedInstanceState, "aircrackWepFragment");
            if (savedInstanceState.containsKey("pcapConcatFragment"))
                pcapConcatFragment = getFragmentManager().getFragment(savedInstanceState, "pcapConcatFragment");
            if (savedInstanceState.containsKey("ivsMergeFragment"))
                ivsMergeFragment = getFragmentManager().getFragment(savedInstanceState, "ivsMergeFragment");
            if (savedInstanceState.containsKey("pcapToIvsFragment"))
                pcapToIvsFragment = getFragmentManager().getFragment(savedInstanceState, "pcapToIvsFragment");
            if (savedInstanceState.containsKey("wpaDictFragment"))
                wpaDictFragment = getFragmentManager().getFragment(savedInstanceState, "wpaDictFragment");
            if (savedInstanceState.containsKey("firmwareFragment"))
                firmwareFragment = getFragmentManager().getFragment(savedInstanceState, "firmwareFragment");
            if (savedInstanceState.containsKey("toolsFragment"))
                toolsFragment = getFragmentManager().getFragment(savedInstanceState, "toolsFragment");
            if (savedInstanceState.containsKey("aboutFragment"))
                aboutFragment = getFragmentManager().getFragment(savedInstanceState, "aboutFragment");
        } else {
            getFragmentManager().beginTransaction().replace(R.id.contentFragment, startFragment).commit();
        }


        menuItems = new ArrayList<>();
        menuItems.add("Start");
        menuItems.add("Wireshark");
        menuItems.add("Airodump");
        menuItems.add("Airdecap");
        menuItems.add("Aircrack");
        menuItems.add("PCAP Merge");
        menuItems.add("IVS Merge");
        menuItems.add("PCAP to IVS converter");
        menuItems.add("WPA Dict Attack");
        menuItems.add("Preferences");
        menuItems.add("Firmware");
        menuItems.add("Tools");
        menuItems.add("About Us");



        adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, menuItems);
        navList.setAdapter(adapter);
        //ActionBar actionBar = getActionBar();

        //actionBar.setDisplayShowTitleEnabled(false);
        //actionBar.setDisplayShowHomeEnabled(false);
        mDrawerToggle = new ActionBarDrawerToggle(this, drawerLayout, R.drawable.ic_menu_white_24dp, R.string.drawer_open, R.string.drawer_close) {
            @Override
            public void onDrawerOpened(View drawerView) {
                super.onDrawerOpened(drawerView);

                invalidateOptionsMenu();
            }

            @Override
            public void onDrawerClosed(View drawerView) {
                super.onDrawerClosed(drawerView);

                invalidateOptionsMenu();
            }
        };

        drawerLayout.setDrawerListener(mDrawerToggle);

        //sharkFragment = new SharkFragment();
        //airodumpFragment = new APfragment();

        navList.setOnItemClickListener(new OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                drawerLayout.closeDrawers();

                switch (position) {
                    case 0:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, startFragment).addToBackStack(null).commit();
                        break;
                    case 1:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, sharkFragment).addToBackStack(null).commit();
                        break;
                    case 2:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, airodumpFragment).addToBackStack(null).commit();
                        break;
                    case 3:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, airdecapFragment).addToBackStack(null).commit();
                        break;
                    case 4:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, aircrackWepFragment).addToBackStack(null).commit();
                        break;
                    case 5:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, pcapConcatFragment).addToBackStack(null).commit();
                        break;
                    case 6:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, ivsMergeFragment).addToBackStack(null).commit();
                        break;
                    case 7:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, pcapToIvsFragment).addToBackStack(null).commit();
                        break;
                    case 8:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, wpaDictFragment).addToBackStack(null).commit();
                        break;
                    case 9:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, settingsFragment).addToBackStack(null).commit();
                        break;
                    case 10:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, firmwareFragment).addToBackStack(null).commit();
                        break;
                    case 11:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, toolsFragment).addToBackStack(null).commit();
                        break;
                    case 12:
                        getFragmentManager().beginTransaction().replace(R.id.contentFragment, aboutFragment).addToBackStack(null).commit();
                        break;

                    default:
                        break;
                }
            }

        });

//
        attackInfoEvaluate(getIntent());
    }


    private boolean isLibsLoaded() {
        if(MyApplication.isLibInstalledCorrectly()) {
            return true;
        } else {
            toast("We could not load all libraries. However, you should be able to install the Nexmon firmware and tools at least.");
            return false;
        }
    }

    private boolean isAccessable() {
        return true;
    }

    private void attackInfoEvaluate(Intent intent) {
        if(intent != null && intent.hasExtra("ATTACK") && intent.hasExtra("ATTACK_TYPE")) {
            String typeString = intent.getStringExtra("ATTACK_TYPE");
            Attack attack = new Gson().fromJson(intent.getStringExtra("ATTACK"), Attack.ATTACK_TYPE.get(typeString));
            attackInfoFragment = AttackInfoFragment.newInstance(attack);
            getFragmentManager().beginTransaction().replace(R.id.contentFragment, attackInfoFragment).commit();

        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Pass the event to ActionBarDrawerToggle
        // If it returns true, then it has handled
        // the nav drawer indicator touch event

        if (mDrawerToggle.onOptionsItemSelected(item)) {
            return true;
        }

        // Handle your other action bar items...

        return super.onOptionsItemSelected(item);
    }

    ;

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        mDrawerToggle.syncState();
    }


    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }

    private void toast(String msg) {
        try {
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
        } catch(Exception e) {e.printStackTrace();}
    }

    @Override
    protected void onStart() {
        super.onStart();
        MyApplication.isAppVisible = true;
        //MyApplication.showSurveyNotification();
    }

    @Override
    protected void onStop() {
        super.onStop();
        MyApplication.isAppVisible = false;
        //MyApplication.dismissSurveyNotification();
    }

    public void setPermissionListener(IPermissionListener permissionListener) {
        this.permissionListener = permissionListener;
    }

    public void removePermissionListener() {
        this.permissionListener = null;
    }

    public interface IPermissionListener {
        void onPermissionResult(int requestCode, String[] permissions, int[] grantResults);
    }
}

