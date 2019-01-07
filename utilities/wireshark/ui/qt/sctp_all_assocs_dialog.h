/* sctp_all_assocs_dialog.h
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

#ifndef SCTP_ALL_ASSOCS_DIALOG_H
#define SCTP_ALL_ASSOCS_DIALOG_H

#include <config.h>

#include <glib.h>

#include <file.h>

#include <epan/dissectors/packet-sctp.h>

#include "ui/tap-sctp-analysis.h"

#include <QDialog>
#include <QObject>

namespace Ui {
class SCTPAllAssocsDialog;
}

class SCTPAllAssocsDialog : public QDialog
{
     Q_OBJECT

public:
    explicit SCTPAllAssocsDialog(QWidget *parent = 0, capture_file *cf = NULL);
    ~SCTPAllAssocsDialog();

    void fillTable();
    sctp_assoc_info_t* getSelectedAssoc() { return selected_assoc; }
    sctp_assoc_info_t* findSelectedAssoc();

public slots:
    void setCaptureFile(capture_file *cf) { cap_file_ = cf; }

private slots:
    void on_analyseButton_clicked();
    void on_setFilterButton_clicked();
    void getSelectedItem();

private:
    Ui::SCTPAllAssocsDialog *ui;
    capture_file *cap_file_;
    sctp_allassocs_info_t *sctp_assocs;
    sctp_assoc_info_t     *selected_assoc;


signals:
    void filterPackets(QString new_filter, bool force);
};

#endif // SCTP_ALL_ASSOCS_DIALOG_H

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
