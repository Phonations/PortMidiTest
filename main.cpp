/**
* Copyright (C) 2012-2014 Phonations
* License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
*/

#include <QCoreApplication>
#include <QDebug>
#include <QThread>

#include <portmidi.h>

PortMidiStream *stream;
int hh1 = 0;
int hh2 = 0;
int mm1 = 0;
int mm2 = 0;
int ss1 = 0;
int ss2 = 0;
int ff1 = 0;
int ff2 = 0;
int rate = 0;

QString conv(int n, int length, int base)
{
	return QString("%1").arg(n, length, base, QChar('0')).toUpper();
}

PmTimestamp proc(void *time_info)
{
	PmEvent event;
	Pm_Read(stream, &event, 1);
	int status = Pm_MessageStatus(event.message);
	int data1 = Pm_MessageData1(event.message);
	int data2 = Pm_MessageData2(event.message);
	int type = data1 >> 4;
	int value = data1 & 0xf;

	switch(type) {
	case 0:
		ff1 = value;
		break;
	case 1:
		ff2 = value;
		break;
	case 2:
		ss1 = value;
		break;
	case 3:
		ss2 = value;
	case 4:
		mm1 = value;
		break;
	case 5:
		mm2 = value;
		break;
	case 6:
		hh1 = value;
		break;
	case 7:
		hh2 = value & 1;
		rate = value >> 1;
		break;
	}

	int ff = (ff2 << 4) + ff1;
	int ss = (ss2 << 4) + ss1;
	int mm = (mm2 << 4) + mm1;
	int hh = (hh2 << 4) + hh1;
	//qDebug() << ss << ff << event.timestamp << QString::number(status, 16)<< QString::number(data1, 2) << type << value << ff1 << ff2 << ff;
	qDebug() << conv(event.timestamp, 8, 10) << conv(data1, 8, 2) << conv(type, 3, 2) << rate << hh << mm << ss << ff;

	return 0;
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	PmError error = Pm_Initialize();
	if(error != pmNoError) {
		qDebug() << "error:" << Pm_GetErrorText(error);
		return error;
	}

	int deviceCount = Pm_CountDevices();
	qDebug() << "count:" << deviceCount;

	for(int i = 0;i < deviceCount; i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		qDebug() << i << info->name << info->input << info->output;
	}

	PmDeviceID id = 1;

	error = Pm_OpenInput(&stream, id, NULL, 0, proc, NULL);

	if(error != pmNoError) {
		qDebug() << "error:" << Pm_GetErrorText(error);
		Pm_Terminate();
		return error;
	}

	a.exec();

	qDebug() << Pm_Close(stream);

	return Pm_Terminate();
}
