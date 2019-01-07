/* packet_format_group_box.h
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef PACKET_FORMAT_GROUP_BOX_H
#define PACKET_FORMAT_GROUP_BOX_H

#include <QGroupBox>

namespace Ui {
class PacketFormatGroupBox;
}

class PacketFormatGroupBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit PacketFormatGroupBox(QWidget *parent = 0);
    ~PacketFormatGroupBox();

    bool summaryEnabled();
    bool detailsEnabled();
    bool bytesEnabled();

    bool allCollapsedEnabled();
    bool asDisplayedEnabled();
    bool allExpandedEnabled();

signals:
    void formatChanged();

private slots:
    void on_detailsCheckBox_toggled(bool checked);
    void on_summaryCheckBox_toggled(bool checked);
    void on_bytesCheckBox_toggled(bool checked);
    void on_allCollapsedButton_toggled(bool checked);
    void on_asDisplayedButton_toggled(bool checked);
    void on_allExpandedButton_toggled(bool checked);

private:
    Ui::PacketFormatGroupBox *pf_ui_;
};

#endif // PACKET_FORMAT_GROUP_BOX_H
