#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include "stdlib.h"
#include "string.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    mDevClassVal(0),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->checkBox_5->setCheckState(Qt::Checked);
    ui->checkBox_7->setCheckState(Qt::Checked);

    on_radioButton_14_toggled(true);
    on_radioButton_31_toggled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::renderDevClassVal_label()
{
    char sVal[16];
    sprintf(sVal, "0x%06X", mDevClassVal);
    ui->label_2->setText(QString::fromUtf8(sVal));
}

void MainWindow::onStateChange(QCheckBox *cb, int position)
{
    unsigned int mask = (1 << position);
    if (cb->isChecked()) {
        this->mDevClassVal |= mask;
    } else {
        this->mDevClassVal &= ~mask;
    }

    this->renderDevClassVal_label();
}


#define DECLARE_CHECKBOX(_num, _bit) void MainWindow::on_checkBox_##_num##_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_##_num, _bit); }
//void MainWindow::on_checkBox_1_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_1, 13); }
//void MainWindow::on_checkBox_2_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_2, 16); }
//void MainWindow::on_checkBox_3_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_3, 17); }
//void MainWindow::on_checkBox_4_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_4, 18); }
//void MainWindow::on_checkBox_5_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_5, 19); }
//void MainWindow::on_checkBox_6_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_6, 20); }
//void MainWindow::on_checkBox_7_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_7, 21); }
//void MainWindow::on_checkBox_8_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_8, 22); }
//void MainWindow::on_checkBox_9_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_9, 23); }
DECLARE_CHECKBOX(1, 13)
DECLARE_CHECKBOX(2, 16)
DECLARE_CHECKBOX(3, 17)
DECLARE_CHECKBOX(4, 18)
DECLARE_CHECKBOX(5, 19)
DECLARE_CHECKBOX(6, 20)
DECLARE_CHECKBOX(7, 21)
DECLARE_CHECKBOX(8, 22)
DECLARE_CHECKBOX(9, 23)

/* Major Device Classes (bits 8-12) value range: [0..31] not a bitmask
12	11	10	9	8	Major Device Class
----------------------------------------------------------------------
0	0	0	0	0	Miscellaneous [Ref #2]
0	0	0	0	1	Computer (desktop, notebook, PDA, organizer, ... )
0	0	0	1	0	Phone (cellular, cordless, pay phone, modem, ...)
0	0	0	1	1	LAN /Network Access point
0	0	1	0	0	Audio/Video (headset, speaker, stereo, video display, VCR, ...
0	0	1	0	1	Peripheral (mouse, joystick, keyboard, ... )
0	0	1	1	0	Imaging (printer, scanner, camera, display, ...)
0	0	1	1	1	Wearable
0	1	0	0	0	Toy
0	1	0	0	1	Health
1	1	1	1	1	Uncategorized: device code not specified
--------------------------------------------------*/
//unsigned int val = 0x1F << 8; \
//mDevClassVal &= ~val; /* clean 5 bits starting from 8th */  \
//mDevClassVal |= (_val << 8); \

#define SET_RADIOBOX_VALUE(_clean_bits, _shift, _num, _val)  \
    unsigned int val = _clean_bits << _shift; \
    mDevClassVal &= ~val; /* clean "_clean_bits" bits starting from "_shift"-th bit*/  \
    mDevClassVal |= (_val << _shift); \
    QRadioButton *pRb = ui->radioButton_##_num;   \
    if (!pRb->isChecked()) \
        pRb->setChecked(true); \
    renderDevClassVal_label();

#define DECLARE_RADIOBOX(_num, _val) void MainWindow::on_radioButton_##_num##_toggled(bool checked) \
{   \
    if (checked) {  \
        SET_RADIOBOX_VALUE(0x1F, 8, _num, _val) \
    }   \
}


DECLARE_RADIOBOX(10, 0)
DECLARE_RADIOBOX(11, 1)
DECLARE_RADIOBOX(12, 2)
DECLARE_RADIOBOX(13, 3)
DECLARE_RADIOBOX(14, 4)
DECLARE_RADIOBOX(15, 5)
DECLARE_RADIOBOX(16, 6)
DECLARE_RADIOBOX(17, 7)
DECLARE_RADIOBOX(18, 8)
DECLARE_RADIOBOX(19, 9)
DECLARE_RADIOBOX(20, 31)


/* Minor Device Class field - Audio/Video Major Class
7	6	5	4	3	2	Minor Device Class bit no. of CoD
----------------------------------------------------------------------
0	0	0	0	0	0	Uncategorized, code not assigned
0	0	0	0	0	1	Wearable Headset Device
0	0	0	0	1	0	Hands-free Device
0	0	0	0	1	1	(Reserved)
0	0	0	1	0	0	Microphone
0	0	0	1	0	1	Loudspeaker
0	0	0	1	1	0	Headphones
0	0	0	1	1	1	Portable Audio
0	0	1	0	0	0	Car audio
0	0	1	0	0	1	Set-top box
0	0	1	0	1	0	HiFi Audio Device
0	0	1	0	1	1	VCR
0	0	1	1	0	0	Video Camera
0	0	1	1	0	1	Camcorder
0	0	1	1	1	0	Video Monitor
0	0	1	1	1	1	Video Display and Loudspeaker
0	1	0	0	0	0	Video Conferencing
0	1	0	0	0	1	(Reserved)
0	1	0	0	1	0	Gaming/Toy
X	X	X	X	X	X	All other values reserved
--------------------------------------------------*/

#define DECLARE_MINOR_DEV_CLASS_HNDL(_num, _val) void MainWindow::on_radioButton_##_num##_toggled(bool checked)\
{   \
    if (checked) {  \
        SET_RADIOBOX_VALUE(0x3F, 2, _num, _val) \
    }   \
}

DECLARE_MINOR_DEV_CLASS_HNDL(21, 1)
DECLARE_MINOR_DEV_CLASS_HNDL(22, 2)
DECLARE_MINOR_DEV_CLASS_HNDL(23, 4)
DECLARE_MINOR_DEV_CLASS_HNDL(24, 5)
DECLARE_MINOR_DEV_CLASS_HNDL(25, 6)
DECLARE_MINOR_DEV_CLASS_HNDL(26, 7)
DECLARE_MINOR_DEV_CLASS_HNDL(27, 8)
DECLARE_MINOR_DEV_CLASS_HNDL(28, 9)
DECLARE_MINOR_DEV_CLASS_HNDL(29, 10)
DECLARE_MINOR_DEV_CLASS_HNDL(30, 11)
DECLARE_MINOR_DEV_CLASS_HNDL(31, 12)
DECLARE_MINOR_DEV_CLASS_HNDL(32, 13)
DECLARE_MINOR_DEV_CLASS_HNDL(33, 14)
DECLARE_MINOR_DEV_CLASS_HNDL(34, 15)
DECLARE_MINOR_DEV_CLASS_HNDL(35, 16)
DECLARE_MINOR_DEV_CLASS_HNDL(36, 18)









