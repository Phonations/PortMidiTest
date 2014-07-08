/**
* Copyright (C) 2012-2014 Phonations
* License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
*/

#include <QCoreApplication>
#include <QDebug>
#include <QThread>

#include <portmidi.h>

#define Pm_MessageData3(msg) (((msg) >> 24) & 0xFF)


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

const char * conv(int n, int length, int base)
{
	return QString("%1").arg(n, length, base, QChar('0')).toUpper().toStdString().c_str();
}

PmTimestamp proc(void *time_info)
{
	PmEvent event;

	int err = Pm_Read(stream, &event, 1);
	if(err < 0)
	{
		qDebug() << "Error while reading the first message" << Pm_GetErrorText((PmError) err);
		return 0;
	}
	int status = Pm_MessageStatus(event.message);

	switch (status) {
	case 0xF0:
	{
		int manufactorID = Pm_MessageData1(event.message);
		int channel = Pm_MessageData2(event.message);
		int type = Pm_MessageData3(event.message);

		switch (type) {
		case 0x01:
		{
			err = Pm_Read(stream, &event, 1);
			if(err < 0)
			{
				qDebug() << "Error while reading the TC message" << Pm_GetErrorText((PmError)err);
				return 0;
			}
			int tcType = Pm_MessageStatus(event.message);
			if(tcType == 1)
			{
				int data1 = Pm_MessageData1(event.message);
				int hh = data1 & 0x1F; // remove the FPS information
				int mm = Pm_MessageData2(event.message);
				int ss = Pm_MessageData3(event.message);

				err = Pm_Read(stream, &event, 1);
				if(err < 0)
				{
					qDebug() << "Error while reading the end of the TC message" << Pm_GetErrorText((PmError)err);
					return 0;
				}
				int ff = Pm_MessageStatus(event.message);
				int eox = Pm_MessageData1(event.message);
				if(eox == 0xF7)
					qDebug() << "Full tc" << hh << mm << ss << ff;
				else
					qDebug() << "Bad TC message";
			}
			else
			{
				qDebug() << "Unknown tc type";
			}
		}
			break;
		default:
			qDebug() << "Unknown SesEx type";
			break;
		}
		break;
	}
		// QF
	case 0xF1:
	{
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
		//qDebug() << "MTC QF" << hh << mm << ss << ff;
		break;
	}
	// Timming clock
	case 0xF8:
		//qDebug() << "timming clock";
		break;

	default:
		qDebug() << conv(event.message, 8, 16) << conv(status, 2, 16);
		break;
	}

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

	PmDeviceID id = 3;

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
