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

int rate = 0;

int ff = 0;
int ss = 0;
int mm = 0;
int hh = 0;

const char * conv(int n, int length, int base)
{
	return QString("%1").arg(n, length, base, QChar('0')).toUpper().toStdString().c_str();
}

const char * displayTC(int hh, int mm, int ss, int ff)
{
	return QString("%1:%2:%3:%4").arg(hh).arg(mm).arg(ss).arg(ff).toStdString().c_str();
}

PmTimestamp proc(void *time_info)
{
	PmEvent events[10];

	int messageCount = Pm_Read(stream, events, 10);
	//qDebug() << messageCount;

	for(int i = 0; i < messageCount; i++)
	{
		int status = Pm_MessageStatus(events[i].message);
		//qDebug() << i << "/" << messageCount;

		switch (status) {
		case 0xF0:
		{
			int manufactorID = Pm_MessageData1(events[i].message);
			int channel = Pm_MessageData2(events[i].message);
			int type = Pm_MessageData3(events[i].message);

			switch (type) {
			case 0x01:
			{
				i++;
				if(i >= messageCount)
				{
					qDebug() << "Error while reading the TC message";
					return 0;
				}
				int tcType = Pm_MessageStatus(events[i].message);
				if(tcType == 1)
				{
					int data1 = Pm_MessageData1(events[i].message);
					hh = data1 & 0x1F; // remove the FPS information
					mm = Pm_MessageData2(events[i].message);
					ss = Pm_MessageData3(events[i].message);

					i++;
					if(i >= messageCount)
					{
						qDebug() << "Error while reading the end of the TC message";
						return 0;
					}
					ff = Pm_MessageStatus(events[i].message);
					int eox = Pm_MessageData1(events[i].message);
					if(eox == 0xF7)
					{
						qDebug() << "Full tc" << displayTC(hh, mm, ss, ff);
					}
					else
						qDebug() << "Bad TC message";
				}
				else
				{
					qDebug() << "Unknown tc type";
				}
				break;
			}
			case 0x07:
			{
				i++;
				if(i >= messageCount)
				{
					qDebug() << "Error while reading the answer message";
					return 0;
				}
				int status = Pm_MessageStatus(events[i].message);
				int data1 = Pm_MessageData1(events[i].message);
				int data2 = Pm_MessageData2(events[i].message);
				int data3 = Pm_MessageData3(events[i].message);
				if(data3 == 0xF7)
				{
					qDebug() << conv(events[i].message, 8, 16) << "Unknown MMC answer" << conv(status, 2, 16);
				}
				else
					qDebug() << conv(events[i].message, 8, 16) << "wrong MMC answer";
				break;

			}
			default:
				qDebug() << conv(events[i].message, 8, 16) << "Unknown SysEx type" << conv(type, 2, 16);
				break;
			}
			break;
		}
			// QF
		case 0xF1:
		{
			int data1 = Pm_MessageData1(events[i].message);
			int type = data1 >> 4;
			int value = data1 & 0xf;

			switch(type) {
			case 0:
				ff = (ff & 0xF0) + value;
				break;
			case 1:
				ff = (value << 4) + (ff & 0x0F) ;
				break;
			case 2:
				ss = (ss & 0xF0) + value;
				break;
			case 3:
				ss = (value << 4) + (ss & 0x0F);
				break;
			case 4:
				mm = (mm & 0xF0) + value;
				break;
			case 5:
				mm = (value << 4) + (mm & 0x0F);
				break;
			case 6:
				hh = (hh & 0xF0) + value;
				break;
			case 7:
				hh = ((value & 1) << 4) + (hh & 0x0F);
				rate = value >> 1;
				break;
			}

			qDebug() << "MTC QF" << conv(events[i].message, 8, 16) << type << displayTC(hh, mm, ss, ff) << conv(data1, 2, 16);
			break;
		}
			// Timming clock
		case 0xF8:
			qDebug() << "timming clock";
			break;

		default:
			qDebug() << conv(events[i].message, 8, 16) << conv(status, 2, 16);
			break;
		}
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

	PmDeviceID id = 2; //3;

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
