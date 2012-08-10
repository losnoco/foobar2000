#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#include "rate/fir_resample.h"
#include "dsp_config.h"
#include "foo_dsp_rate.h"
#include "dsp_config.h"

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef _DEBUG
#include <stdio.h>
#endif

#define STDAFX
