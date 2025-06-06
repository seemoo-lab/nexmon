/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * License:                                                                *
 *                                                                         *
 * Copyright (c) 2017 Secure Mobile Networking Lab (SEEMOO)                *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * 1. The above copyright notice and this permission notice shall be       *
 *    include in all copies or substantial portions of the Software.       *
 *                                                                         *
 * 2. Any use of the Software which results in an academic publication or  *
 *    other publication which includes a bibliography must include         *
 *    citations to the nexmon project a) and the paper cited under b):     *
 *                                                                         *
 *    a) "Matthias Schulz, Daniel Wegemer and Matthias Hollick. Nexmon:    *
 *        The C-based Firmware Patching Framework. https://nexmon.org"     *
 *                                                                         *
 *    b) "Matthias Schulz, Francesco Gringoli, Daniel Steinmetzer, Michael *
 *        Koch and Matthias Hollick. Massive Reactive Smartphone-Based     *
 *        Jamming using Arbitrary Waveforms and Adaptive Power Control.    *
 *        Proceedings of the 10th ACM Conference on Security and Privacy   *
 *        in Wireless and Mobile Networks (WiSec 2017), July 2017."        *
 *                                                                         *
 * 3. The Software is not used by, in cooperation with, or on behalf of    *
 *    any armed forces, intelligence agencies, reconnaissance agencies,    *
 *    defense agencies, offense agencies or any supplier, contractor, or   *
 *    research associated.                                                 *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 *                                                                         *
 **************************************************************************/

package de.tu_darmstadt.seemoo.nexmon.utils;

import android.content.Context;
import android.util.Log;

import java.util.List;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import eu.chainfire.libsuperuser.Shell;

/**
 * Created by matthias on 24.05.17.
 */

public class Dhdutil {
    protected static Dhdutil instance;
    private String dhdutilPath;

    protected Dhdutil() {
        this.instance = this;
        dhdutilPath = Assets.getAssetsPath(MyApplication.getAppContext(), "dhdutil");
    }

    public static Dhdutil getInstance() {
        return instance == null ? new Dhdutil() : instance;
    }

    public String dumpConsole() {
        List<String> out = Shell.SU.run(dhdutilPath + " consoledump");
        StringBuilder sb = new StringBuilder();
        for(String str : out) {
            sb.append(str);
            sb.append("\n");
        }
        return sb.toString();
    }
}
