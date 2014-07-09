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
	PmEvent event;

	bool reading = true;
	while(reading)
	{
		int messageRead = Pm_Read(stream, &event, 1);
		if(messageRead <= 0)
			reading = false;
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
				messageRead = Pm_Read(stream, &event, 1);
				if(messageRead < 0)
				{
					qDebug() << "Error while reading the TC message" << Pm_GetErrorText((PmError)messageRead);
					return 0;
				}
				int tcType = Pm_MessageStatus(event.message);
				if(tcType == 1)
				{
					int data1 = Pm_MessageData1(event.message);
					hh = data1 & 0x1F; // remove the FPS information
					mm = Pm_MessageData2(event.message);
					ss = Pm_MessageData3(event.message);

					messageRead = Pm_Read(stream, &event, 1);
					if(messageRead < 0)
					{
						qDebug() << "Error while reading the end of the TC message" << Pm_GetErrorText((PmError)messageRead);
						return 0;
					}
					ff = Pm_MessageStatus(event.message);
					int eox = Pm_MessageData1(event.message);
					if(eox == 0xF7)
					{
						// we SUPPOSE that the next message is garbage
						reading = false;
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
				messageRead = Pm_Read(stream, &event, 1);
				if(messageRead < 0)
				{
					qDebug() << "Error while reading the answer message" << Pm_GetErrorText((PmError)messageRead);
					return 0;
				}
				int status = Pm_MessageStatus(event.message);
				int data1 = Pm_MessageData1(event.message);
				int data2 = Pm_MessageData2(event.message);
				int data3 = Pm_MessageData3(event.message);
				if(data3 == 0xF7)
				{
					qDebug() << conv(event.message, 8, 16) << "Unknown MMC answer" << conv(status, 2, 16);
				}
				else
					qDebug() << conv(event.message, 8, 16) << "wrong MMC answer";
				break;

			}
			default:
				qDebug() << conv(event.message, 8, 16) << "Unknown SysEx type" << conv(type, 2, 16);
				break;
			}
			break;
		}
			// QF
		case 0xF1:
		{
			int data1 = Pm_MessageData1(event.message);
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
			qDebug() << conv(event.message, 8, 16) << conv(status, 2, 16);
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
