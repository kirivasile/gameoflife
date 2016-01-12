#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <mpi.h>
#include <stdio.h>

enum messageType {
	FIELD_DATA, //Отправка рабочим данных о поле
	UP_DATA, //Отправка рабочему вниз (т.е. updata = данные сверху)
	DOWN_DATA, //Отправка рабочему вверх (т.е. downdata = данные снизу)
	FINISH_BORDERS, //Отправка мастеру границ рабочего
	FINISH_DATA, //Отправка мастеру результата
	STOP_SGN //Стоп-сигнал мастера
};
