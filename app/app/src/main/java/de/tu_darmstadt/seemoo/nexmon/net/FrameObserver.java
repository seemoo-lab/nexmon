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

package de.tu_darmstadt.seemoo.nexmon.net;

import java.util.concurrent.CopyOnWriteArrayList;

import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;

public class FrameObserver implements IFrameReceiver {
    private CopyOnWriteArrayList<IFrameReceiver> frameObserver;


    public FrameObserver() {
        frameObserver = new CopyOnWriteArrayList<IFrameReceiver>();
    }

    public void addObserver(IFrameReceiver frameObserver) {
        if (!this.frameObserver.contains(frameObserver)) {
            this.frameObserver.add(frameObserver);
        }
    }

    public void removeObserver(IFrameReceiver frameObserver) {
        this.frameObserver.remove(frameObserver);
    }

    public void removeAllObserver() {
        frameObserver.clear();
    }


    @Override
    public void packetReceived(Packet packet) {
        for (IFrameReceiver observer : frameObserver)
            observer.packetReceived(packet);
    }


}
