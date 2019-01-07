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
import android.support.v4.app.Fragment;

import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link AttackDialog#newInstance} factory method to
 * create an instance of this fragment.
 */
public abstract class AttackDialog extends DialogFragment {

    protected final static String ACCESSPOINT = "AccessPoint";
    protected AccessPoint ap;

}
