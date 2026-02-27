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

import android.app.Activity;
import android.app.Fragment;
import android.content.Intent;
import android.os.Bundle;

import com.google.gson.Gson;

import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;

/**
 * Created by fabian on 10/4/16.
 */
public class AttackInfoActivity extends Activity {

    Fragment attackInfoFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_info);

        attackInfoEvaluate(getIntent());
        try {
            setTitle("Nexmon: Attack Info");
        } catch(Exception e) {e.printStackTrace();}
    }

    private void attackInfoEvaluate(Intent intent) {
        if(intent != null && intent.hasExtra("ATTACK") && intent.hasExtra("ATTACK_TYPE")) {
            String typeString = intent.getStringExtra("ATTACK_TYPE");
            Attack attack = new Gson().fromJson(intent.getStringExtra("ATTACK"), Attack.ATTACK_TYPE.get(typeString));
            attackInfoFragment = AttackInfoFragment.newInstance(attack);
            getFragmentManager().beginTransaction().replace(R.id.attackInfoFragment, attackInfoFragment).commit();

        }
    }
}
