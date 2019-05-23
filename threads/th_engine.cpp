#include "../main.h"

//#pragma region Engine
PI_THREAD(EngineWatch)
{
	Settings* settings = Settings::getInstance();
	if (!settings->threadFlag.EngineWatch) return (void*)0;
	bool engineStatus = false;
	bool stopFailed = 0;
	int requestCount = 3;
	int timeout = 500;

	Database* db = new Database();
	db->Init(settings);
	if (db->Open())
		printf("IB ERROR: %s\n", db->lastErrorMessage);
	char myNote[] = "[THREAD] Engine: Engine control thread init";
	if (db->Log(DB_EVENT_TYPE_THREAD_INIT, 0, 0, myNote))
		printf("IB ERROR: %s\n", db->lastErrorMessage);

	settings->workFlag.EngineWatch = 0;
	engine->Init(settings);
	if (!engine->OpenDevice())
		printf("[ENG]: Communication port error!\n");

	settings->workFlag.EngineWatch = 0;
	engineStatus = false;
		timeout = requestCount;
	while (!engineStatus && (timeout-->0))
		{ engineStatus = engine->engineUpdate(); delay_ms(100); }
	engineStatus = false;
	timeout = requestCount;
		settings->workFlag.EngineWatch = 0;
	while (!engineStatus && (timeout-->0))
	{
		engineStatus |=	engine->engineStart(0);
		engineStatus |=	engine->engineStop();
		if (!engineStatus) delay(1);
	}

	term_setattr(31);
	if (timeout == 0) printf("DBG:> [ENG] Engine INIT stop FAILED!!!\n");
	term_setattr(0);
	int bypassPinNum = settings->getPinConfig(DVC_SENSOR_BYPASS, 1);
	if (!settings->getEnabledDevice(DVC_SENSOR_BYPASS)) bypassPinNum = 0xFF;
	while (settings->threadFlag.EngineWatch)
	{
		settings->workFlag.EngineWatch = 0;
		timeout = 500;
		while ((settings->busyFlag.EngineWatch) && (timeout-- > 0)) {delay_ms(1);}
		settings->busyFlag.EngineWatch++;
        engineStatus = false;
		timeout = requestCount;
		while (!engineStatus && (timeout-->0))
			engineStatus = engine->engineUpdate();
		settings->intErrorCode.EngineWatch = (engine->errorCode == 0)?0:201;

		if (
			(engine->currFreq > 0) &&
			(
				(engine->needFreq < 1) ||
				(
					(
						(status.intDeviceInfo.money_currentBalance < 1) ||
						(status.intDeviceInfo.program_currentProgram < 1)
					) && (winterModeEngineActive == 0)
				)
			)
		   )
		{
			if (settings->debugFlag.EngineWatch)
				printf("DBG:> [%3d:%02d:%02d] [ENG] STOP [c: %d; n: %d; bal: %d]\n",
					((long)(get_prguptime()/3600)), ((long)(get_prguptime()/60))%60, get_prguptime()%60 ,
					engine->currFreq, engine->needFreq, status.intDeviceInfo.money_currentBalance);
			engineStatus = false;
			timeout = requestCount;
			while (!engineStatus && (timeout-->0))
			{
				engine->needFreq = 1;
				engineStatus |=	engine->engineStart(1);
				delay_ms(200);
				engineStatus |=	engine->engineStop();
				delay_ms(settings->commonParams.engine_StartStopTimeMs);
			}
			term_setattr(31);
			if (timeout == 0)
				if (settings->debugFlag.EngineWatch) printf("DBG:> [ENG] Timeout [FLAG: 9]\n");
			term_setattr(0);
		}

        if ((engine->currFreq != engine->needFreq) || (getGPIOState(bypassPinNum) == 0))
		{
			if (engine->needFreq > 0)
			{
				if (settings->debugFlag.EngineWatch)
					printf("DBG:> [%3d:%02d:%02d] [ENG] START [p:%d c:%d; n:%d; bal:%d; bp:%d]\n",
						((long)(get_prguptime()/3600)), ((long)(get_prguptime()/60))%60, get_prguptime()%60,
						status.intDeviceInfo.program_currentProgram, engine->currFreq, engine->needFreq, status.intDeviceInfo.money_currentBalance, (getGPIOState(bypassPinNum) == 0));
				if (getGPIOState(bypassPinNum) == 0)
				{
					engineStatus = false;
					timeout = requestCount;
					while (!engineStatus && (timeout-->0))
						engineStatus = engine->engineStart(500);
					if (timeout == 0)
						if (settings->debugFlag.EngineWatch) printf("[DEBUG] >> [ENG] Timeout [FLAG: 1]\n");
					delay_ms(settings->commonParams.engine_StartStopTimeMs);
				}
				else
				{
					engineStatus = false;
					timeout = requestCount;
					while (!engineStatus && (timeout-->0))
						engineStatus = engine->engineStart(engine->needFreq);
					term_setattr(31);
					if (timeout == 0)
						if (settings->debugFlag.EngineWatch) printf("[DEBUG] >> [ENG] Timeout [FLAG: 2]\n");
					term_setattr(0);
					delay_ms(settings->commonParams.engine_StartStopTimeMs);
				}
			}
		}

		settings->busyFlag.EngineWatch--;
		delay_ms(200);
	}
	engine->engineStop();
	engine->CloseDevice();
	printf("[ENG]: Thread ended.\n");
	return (void*)0;
}
//#pragma endregion
