/* bluetooth_device_dialog.cpp
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

#include "bluetooth_device_dialog.h"
#include <ui_bluetooth_device_dialog.h>

#include "epan/epan.h"
#include "epan/addr_resolv.h"
#include "epan/to_str.h"
#include "epan/epan_dissect.h"
#include "epan/dissectors/packet-bthci_cmd.h"
#include "epan/dissectors/packet-bthci_evt.h"

#include "ui/simple_dialog.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QTreeWidget>
#include <QFileDialog>

static const int column_number_value = 0;
static const int column_number_changes = 1;

static const int row_number_bd_addr = 0;
static const int row_number_bd_addr_oui = 1;
static const int row_number_name = 2;
static const int row_number_class_of_device = 3;
static const int row_number_lmp_version = 4;
static const int row_number_lmp_subversion = 5;
static const int row_number_manufacturer = 6;
static const int row_number_hci_version = 7;
static const int row_number_hci_revision = 8;
static const int row_number_scan = 9;
static const int row_number_authentication = 10;
static const int row_number_encryption = 11;
static const int row_number_acl_mtu = 12;
static const int row_number_acl_packets = 13;
static const int row_number_sco_mtu = 14;
static const int row_number_sco_packets = 15;
static const int row_number_le_acl_mtu = 16;
static const int row_number_le_acl_packets = 17;
static const int row_number_inquiry_mode = 18;
static const int row_number_page_timeout = 19;
static const int row_number_simple_pairing_mode = 20;
static const int row_number_voice_setting = 21;

typedef struct _item_data_t {
        guint32  interface_id;
        guint32  adapter_id;
        guint32  frame_number;
        gint     changes;
} item_data_t;

Q_DECLARE_METATYPE(item_data_t *)

static gboolean
bluetooth_device_tap_packet(void *tapinfo_ptr, packet_info *pinfo, epan_dissect_t *edt, const void* data)
{
    bluetooth_device_tapinfo_t *tapinfo = (bluetooth_device_tapinfo_t *) tapinfo_ptr;

    if (tapinfo->tap_packet)
        tapinfo->tap_packet(tapinfo, pinfo, edt, data);

    return TRUE;
}

static void
bluetooth_device_tap_reset(void *tapinfo_ptr)
{
    bluetooth_device_tapinfo_t *tapinfo = (bluetooth_device_tapinfo_t *) tapinfo_ptr;

    if (tapinfo->tap_reset)
        tapinfo->tap_reset(tapinfo);
}


static void
bluetooth_devices_tap(void *data)
{
    GString *error_string;

    error_string = register_tap_listener("bluetooth.device", data, NULL,
            0,
            bluetooth_device_tap_reset,
            bluetooth_device_tap_packet,
            NULL
            );

    if (error_string != NULL) {
        simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
                "%s", error_string->str);
        g_string_free(error_string, TRUE);
    }
}


BluetoothDeviceDialog::BluetoothDeviceDialog(QWidget &parent, CaptureFile &cf, QString bdAddr, QString name, guint32 interface_id, guint32 adapter_id, gboolean is_local) :
    WiresharkDialog(parent, cf),
    ui(new Ui::BluetoothDeviceDialog)
{
    QString titleBdAddr;
    QString titleName;

    ui->setupUi(this);
    resize(parent.width() * 4 / 10, parent.height() * 2 / 2);

    setTitle(bdAddr, name);

    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(tableContextMenu(const QPoint &)));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    ui->tableWidget->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
#else
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
#endif

    context_menu_.addActions(QList<QAction *>() << ui->actionCopy_Cell);
    context_menu_.addActions(QList<QAction *>() << ui->actionCopy_Rows);
    context_menu_.addActions(QList<QAction *>() << ui->actionCopy_All);
    context_menu_.addActions(QList<QAction *>() << ui->actionSave_as_image);

    changes_ = 0;

    tapinfo_.tap_packet   = tapPacket;
    tapinfo_.tap_reset    = tapReset;
    tapinfo_.ui           = this;
    tapinfo_.is_local     = is_local;
    tapinfo_.bdAddr       = bdAddr;
    tapinfo_.interface_id = interface_id;
    tapinfo_.adapter_id   = adapter_id;
    tapinfo_.changes      = &changes_;

    ui->hintLabel->setText(ui->hintLabel->text().arg(changes_));

    for (int i_row = 0; i_row < ui->tableWidget->rowCount(); i_row += 1) {
        for (int i_column = 0; i_column < ui->tableWidget->columnCount(); i_column += 1) {
            QTableWidgetItem *item = new QTableWidgetItem();
            ui->tableWidget->setItem(i_row, i_column, item);
        }
    }

    bluetooth_devices_tap(&tapinfo_);

    cap_file_.retapPackets();
}


BluetoothDeviceDialog::~BluetoothDeviceDialog()
{
    delete ui;

    remove_tap_listener(&tapinfo_);
}

void BluetoothDeviceDialog::setTitle(QString bdAddr, QString name)
{
    QString titleBdAddr;
    QString titleName;

    if (bdAddr.isEmpty())
        titleBdAddr = QString(tr("Unknown"));
    else
        titleBdAddr = bdAddr;

    if (name.isEmpty())
        titleName = "";
    else
        titleName = " ("+name+")";

    setWindowTitle(tr("Bluetooth Device - %1%2").arg(titleBdAddr).arg(titleName));
}

void BluetoothDeviceDialog::captureFileClosing()
{
    remove_tap_listener(&tapinfo_);

    WiresharkDialog::captureFileClosing();
}


void BluetoothDeviceDialog::changeEvent(QEvent *event)
{
    if (0 != event)
    {
        switch (event->type())
        {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
        }
    }
    QDialog::changeEvent(event);
}


void BluetoothDeviceDialog::keyPressEvent(QKeyEvent *)
{
/* NOTE: Do nothing, but in real it "takes focus" from button_box so allow user
 * to use Enter button to jump to frame from table widget */
}


void BluetoothDeviceDialog::tableContextMenu(const QPoint &pos)
{
    context_menu_.exec(ui->tableWidget->viewport()->mapToGlobal(pos));
}

void BluetoothDeviceDialog::on_actionCopy_Cell_triggered()
{
    QClipboard             *clipboard = QApplication::clipboard();
    QString                 copy;

    copy = QString(ui->tableWidget->currentItem()->text());

    clipboard->setText(copy);
}

void BluetoothDeviceDialog::on_actionCopy_Rows_triggered()
{
    QClipboard                         *clipboard = QApplication::clipboard();
    QString                             copy;
    QList<QTableWidgetItem *>           items;
    QList<QTableWidgetItem *>::iterator i_item;

    items =  ui->tableWidget->selectedItems();

    for (i_item = items.begin(); i_item != items.end(); ++i_item) {
        copy += QString("%1  %2  %3\n")
                .arg(ui->tableWidget->verticalHeaderItem((*i_item)->row())->text(), -40)
                .arg(ui->tableWidget->item((*i_item)->row(), column_number_value)->text(), -50)
                .arg(ui->tableWidget->item((*i_item)->row(), column_number_changes)->text(), -10);
    }

    clipboard->setText(copy);
}

void BluetoothDeviceDialog::on_actionCopy_All_triggered()
{
    QClipboard             *clipboard = QApplication::clipboard();
    QString                 copy;

    copy += QString("%1  %2  %3\n")
            .arg("Headers", -40)
            .arg(ui->tableWidget->horizontalHeaderItem(column_number_value)->text(), -50)
            .arg(ui->tableWidget->horizontalHeaderItem(column_number_changes)->text(), -10);

    for (int i_row = 0; i_row < ui->tableWidget->rowCount(); i_row += 1) {
        for (int i_column = 0; i_column < ui->tableWidget->columnCount(); i_column += 1) {

        copy += QString("%1  %2  %3\n")
                .arg(ui->tableWidget->verticalHeaderItem(i_row)->text(), -40)
                .arg(ui->tableWidget->item(i_row, column_number_value)->text(), -50)
                .arg(ui->tableWidget->item(i_row, column_number_changes)->text(), -10);
        }
    }

    clipboard->setText(copy);
}



void BluetoothDeviceDialog::tapReset(void *tapinfo_ptr)
{
    bluetooth_device_tapinfo_t *tapinfo = (bluetooth_device_tapinfo_t *) tapinfo_ptr;
    BluetoothDeviceDialog  *dialog = static_cast<BluetoothDeviceDialog *>(tapinfo->ui);

    for (int i_row = 0; i_row < dialog->ui->tableWidget->rowCount(); i_row += 1) {
        for (int i_column = 0; i_column < dialog->ui->tableWidget->columnCount(); i_column += 1) {
            QTableWidgetItem *item = new QTableWidgetItem();
            dialog->ui->tableWidget->setItem(i_row, i_column, item);
        }
    }
    *tapinfo->changes = 0;
}

void BluetoothDeviceDialog::updateChanges(QTableWidget *tableWidget, QString value, const int row, guint *changes, packet_info *pinfo)
{
    QTableWidgetItem *item = tableWidget->item(row, column_number_value);
    item_data_t *item_data = item->data(Qt::UserRole).value<item_data_t *>();

    if (item->text() == value)
        return;

    if (item_data->changes == -1) {
        item_data->changes = 0;
    } else {
        *changes += 1;
        item_data->changes += 1;
        item_data->frame_number = pinfo->fd->num;
        tableWidget->item(row, column_number_changes)->setText(QString::number(item_data->changes));
    }
}

void BluetoothDeviceDialog::saveItemData(QTableWidgetItem *item,
        bluetooth_device_tap_t *tap_device, packet_info *pinfo)
{
    if (item->data(Qt::UserRole).isValid())
        return;

    item_data_t *item_data = wmem_new(wmem_file_scope(), item_data_t);
    item_data->interface_id = tap_device->interface_id;
    item_data->adapter_id = tap_device->adapter_id;
    item_data->changes = -1;
    item_data->frame_number = pinfo->fd->num;
    item->setData(Qt::UserRole, QVariant::fromValue<item_data_t *>(item_data));

}

gboolean BluetoothDeviceDialog::tapPacket(void *tapinfo_ptr, packet_info *pinfo, epan_dissect_t *, const void *data)
{
    bluetooth_device_tapinfo_t   *tapinfo    = static_cast<bluetooth_device_tapinfo_t *>(tapinfo_ptr);
    BluetoothDeviceDialog        *dialog     = static_cast<BluetoothDeviceDialog *>(tapinfo->ui);
    bluetooth_device_tap_t       *tap_device = static_cast<bluetooth_device_tap_t *>(const_cast<void *>(data));
    QString                       bd_addr;
    QString                       bd_addr_oui;
    QString                       name;
    const gchar                  *manuf;
    QTableWidget                 *tableWidget;
    QTableWidgetItem             *item;
    QString                       field;

    tableWidget = dialog->ui->tableWidget;

    if (!((!tap_device->is_local && tap_device->has_bd_addr) || (tap_device->is_local && tapinfo->is_local && tap_device->interface_id == tapinfo->interface_id && tap_device->adapter_id == tapinfo->adapter_id))) {
        return TRUE;
    }

    if (!tap_device->is_local && tap_device->has_bd_addr) {
        bd_addr.sprintf("%02x:%02x:%02x:%02x:%02x:%02x", tap_device->bd_addr[0], tap_device->bd_addr[1], tap_device->bd_addr[2], tap_device->bd_addr[3], tap_device->bd_addr[4], tap_device->bd_addr[5]);

        if (bd_addr != tapinfo->bdAddr)
            return TRUE;
    }

    if (tap_device->has_bd_addr) {
        bd_addr.sprintf("%02x:%02x:%02x:%02x:%02x:%02x", tap_device->bd_addr[0], tap_device->bd_addr[1], tap_device->bd_addr[2], tap_device->bd_addr[3], tap_device->bd_addr[4], tap_device->bd_addr[5]);
        manuf = get_ether_name(tap_device->bd_addr);
        if (manuf) {
            int pos;

            bd_addr_oui = QString(manuf);
            pos = bd_addr_oui.indexOf('_');
            if (pos < 0) {
                manuf = NULL;
            } else {
                bd_addr_oui.remove(pos, bd_addr_oui.size());
            }
        }

        if (!manuf)
            bd_addr_oui = "";

        item = tableWidget->item(row_number_bd_addr, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, bd_addr, row_number_bd_addr, tapinfo->changes, pinfo);
        item->setText(bd_addr);

        item = tableWidget->item(row_number_bd_addr_oui, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, bd_addr_oui, row_number_bd_addr_oui, tapinfo->changes, pinfo);
        item->setText(bd_addr_oui);

        dialog->setTitle(bd_addr, tableWidget->item(row_number_name, column_number_value)->text());
    }

    switch (tap_device->type) {
    case BLUETOOTH_DEVICE_LOCAL_ADAPTER:
    case BLUETOOTH_DEVICE_BD_ADDR:
        break;
    case BLUETOOTH_DEVICE_NAME:
        item = tableWidget->item(row_number_name, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, QString(tap_device->data.name), row_number_name, tapinfo->changes, pinfo);

        item->setText(tap_device->data.name);

        dialog->setTitle(tableWidget->item(row_number_bd_addr, column_number_value)->text(), tap_device->data.name);

        break;
    case BLUETOOTH_DEVICE_RESET:
        for (int i_row = 0; i_row < dialog->ui->tableWidget->rowCount(); i_row += 1) {
            QTableWidgetItem  *item;
            item_data_t       *item_data;

            item = dialog->ui->tableWidget->item(i_row, column_number_value);
            saveItemData(item, tap_device, pinfo);

            item_data = item->data(Qt::UserRole).value<item_data_t *>();

            if (item_data->changes > -1) {
                item_data->changes += 1;
                item_data->frame_number = pinfo->fd->num;
                dialog->ui->tableWidget->item(i_row, column_number_changes)->setText(QString::number(item_data->changes));
            } else {
                item_data->changes = 0;
            }
            dialog->ui->tableWidget->item(i_row, column_number_value)->setText("");
        }
        *tapinfo->changes += 1;

        break;
    case BLUETOOTH_DEVICE_SCAN:
        field = QString(val_to_str_const(tap_device->data.scan, bthci_cmd_scan_enable_values, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_scan, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_scan, tapinfo->changes, pinfo);
        item->setText(field);
        break;
    case BLUETOOTH_DEVICE_LOCAL_VERSION:
        field = QString(val_to_str_const(tap_device->data.local_version.hci_version, bthci_evt_hci_version, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_hci_version, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_hci_version, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString("").sprintf("%u", tap_device->data.local_version.hci_revision);
        item = tableWidget->item(row_number_hci_revision, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_hci_revision, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString(val_to_str_const(tap_device->data.local_version.lmp_version, bthci_evt_lmp_version, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_lmp_version, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_lmp_version, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString(val_to_str_const(tap_device->data.local_version.lmp_version, bthci_evt_lmp_version, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_lmp_version, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_lmp_version, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString("").sprintf("%u", tap_device->data.local_version.lmp_subversion);
        item = tableWidget->item(row_number_lmp_subversion, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_lmp_subversion, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString(val_to_str_ext_const(tap_device->data.local_version.manufacturer, &bluetooth_company_id_vals_ext, "Unknown 0x%04x"));
        item = tableWidget->item(row_number_manufacturer, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_manufacturer, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_REMOTE_VERSION:
        field = QString(val_to_str_const(tap_device->data.remote_version.lmp_version, bthci_evt_lmp_version, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_lmp_version, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_lmp_version, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString("").sprintf("%u", tap_device->data.remote_version.lmp_subversion);
        item = tableWidget->item(row_number_lmp_subversion, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_lmp_subversion, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString(val_to_str_ext_const(tap_device->data.remote_version.manufacturer, &bluetooth_company_id_vals_ext, "Unknown 0x%04x"));
        item = tableWidget->item(row_number_manufacturer, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_manufacturer, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_VOICE_SETTING:
        field = QString("").sprintf("0x%04x", tap_device->data.voice_setting);
        item = tableWidget->item(row_number_voice_setting, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_voice_setting, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_CLASS_OF_DEVICE:
        field = QString("").sprintf("0x%06x", tap_device->data.class_of_device);
        item = tableWidget->item(row_number_class_of_device, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_class_of_device, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_AUTHENTICATION:
        field = QString(val_to_str_const(tap_device->data.authentication, bthci_cmd_authentication_enable_values, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_authentication, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_authentication, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_ENCRYPTION:
        field = QString(val_to_str_const(tap_device->data.encryption, bthci_cmd_encrypt_mode_vals, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_encryption, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_encryption, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_SIMPLE_PAIRING_MODE:
        field = QString(tap_device->data.encryption ? tr("enabled") : tr("disabled"));
        item = tableWidget->item(row_number_simple_pairing_mode, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_simple_pairing_mode, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_PAGE_TIMEOUT:
        field = QString(tr("%1 ms (%2 slots)")).arg(tap_device->data.page_timeout * 0.625).arg(tap_device->data.page_timeout);
        item = tableWidget->item(row_number_page_timeout, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_page_timeout, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_INQUIRY_MODE:
        field = QString(val_to_str_const(tap_device->data.inquiry_mode, bthci_cmd_inq_modes, "Unknown 0x%02x"));
        item = tableWidget->item(row_number_inquiry_mode, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_inquiry_mode, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_MTUS:
        field = QString::number(tap_device->data.mtus.acl_mtu);
        item = tableWidget->item(row_number_acl_mtu, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_acl_mtu, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString::number(tap_device->data.mtus.acl_packets);
        item = tableWidget->item(row_number_acl_packets, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_acl_packets, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString::number(tap_device->data.mtus.sco_mtu);
        item = tableWidget->item(row_number_sco_mtu, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_sco_mtu, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString::number(tap_device->data.mtus.sco_packets);
        item = tableWidget->item(row_number_sco_packets, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_sco_packets, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    case BLUETOOTH_DEVICE_LE_MTU:
        field = QString::number(tap_device->data.le_mtus.acl_mtu);
        item = tableWidget->item(row_number_le_acl_mtu, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_le_acl_mtu, tapinfo->changes, pinfo);
        item->setText(field);

        field = QString::number(tap_device->data.le_mtus.acl_packets);
        item = tableWidget->item(row_number_le_acl_packets, column_number_value);
        saveItemData(item, tap_device, pinfo);
        updateChanges(tableWidget, field, row_number_le_acl_packets, tapinfo->changes, pinfo);
        item->setText(field);

        break;
    }

    dialog->ui->hintLabel->setText(QString(tr("%1 changes")).arg(*tapinfo->changes));

    return TRUE;
}

void BluetoothDeviceDialog::interfaceCurrentIndexChanged(int)
{
    cap_file_.retapPackets();
}

void BluetoothDeviceDialog::showInformationStepsChanged(int)
{
    cap_file_.retapPackets();
}


void BluetoothDeviceDialog::on_tableWidget_itemActivated(QTableWidgetItem *item)
{
    if (!cap_file_.isValid())
        return;

    if (!item->data(Qt::UserRole).isValid())
        return;

    item_data_t *item_data = item->data(Qt::UserRole).value<item_data_t *>();

    emit goToPacket(item_data->frame_number);

}

void BluetoothDeviceDialog::on_actionSave_as_image_triggered()
{
    QPixmap image;

    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Table Image"),
            "bluetooth_device_table.png",
            tr("PNG Image (*.png)"));

    if (fileName.isEmpty()) return;

    image = QPixmap::grabWidget(ui->tableWidget);
    image.save(fileName, "PNG");
}

void BluetoothDeviceDialog::on_buttonBox_clicked(QAbstractButton *)
{

}

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
