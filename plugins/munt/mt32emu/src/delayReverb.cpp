/* Copyright (C) 2003-2009 Dean Beeler, Jerome Fisher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "mt32emu.h"

#include "delayReverb.h"

using namespace MT32Emu;

// All in seconds
const float RAMP_TIME = 1.0f / 88.0f; // Time taken to ramp up from 0 to desired reverb/feedback levels after parameter change
const float BASE_DELAY = 0.0006875;
const float LEFT_DELAY_COEF = 0.056;
const float RIGHT_DELAY_COEF = 0.028;

DelayReverb::DelayReverb() {
	sampleRate = 0;
	buf = NULL;
	bufSize = 0;
	leftDelaySeconds = 0;
	rightDelaySeconds = 0;
	targetReverbLevel = 0;
	targetFeedbackLevel = 0;
}

DelayReverb::~DelayReverb() {
	delete[] buf;
}

void DelayReverb::setSampleRate(unsigned int sampleRate) {
	if(sampleRate != this->sampleRate) {
		this->sampleRate = sampleRate;
		delete[] buf;
		// FIXME: Always 2 second buffer - we could reduce this to what we actually need after we've tweaked the parameters
		bufSize = 2 * sampleRate;
		buf = new float[bufSize];
		this->rampTarget = (unsigned int)(RAMP_TIME * sampleRate);
		reset();
	}
}

void DelayReverb::setParameters(Bit8u mode, Bit8u time, Bit8u level) {
	float oldLeftDelaySeconds = leftDelaySeconds;
	float oldRightDelaySeconds = rightDelaySeconds;
	float oldTargetReverbLevel = targetReverbLevel;
	float oldTargetFeedbackLevel = targetFeedbackLevel;

	leftDelaySeconds = BASE_DELAY + time * LEFT_DELAY_COEF;
	rightDelaySeconds = BASE_DELAY + time * RIGHT_DELAY_COEF;
	targetReverbLevel = level * 6.0f / 127.0f;
	targetFeedbackLevel = 30.0f / 128.0f;

	if (leftDelaySeconds != oldLeftDelaySeconds || rightDelaySeconds != oldRightDelaySeconds || targetReverbLevel != oldTargetReverbLevel || targetFeedbackLevel != oldTargetFeedbackLevel) {
		resetParameters();
	}
}

void DelayReverb::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, long numSamples) {
	for (unsigned int sampleIx = 0; sampleIx < numSamples; sampleIx++) {
		float leftSample = inLeft[sampleIx];
		float rightSample = inRight[sampleIx];

		bufIx = (bufSize + bufIx - 1) % bufSize;
		float reverbLeft = buf[(bufIx + leftDelay) % bufSize];
		float reverbRight = buf[(bufIx + rightDelay) % bufSize];

		outLeft[sampleIx] = (reverbLeft * reverbLevel) + leftSample;
		outRight[sampleIx] = (reverbRight * reverbLevel) + rightSample;

		buf[bufIx] = (reverbLeft * feedbackLevel) + (leftSample + rightSample) / 2.0f;

		if (rampCount < rampTarget) {
			// Linearly ramp up reverb/feedback levels over RAMP_TIME (after parameter change)
			rampCount++;
			if (rampCount == rampTarget) {
				reverbLevel = targetReverbLevel;
				feedbackLevel = targetFeedbackLevel;
			} else {
				reverbLevel += reverbLevelRampInc;
				feedbackLevel += feedbackLevelRampInc;
			}
		}
	}
}

void DelayReverb::reset() {
	resetBuffer();
	resetParameters();
}

void DelayReverb::resetBuffer() {
	bufIx = 0;
	if (buf != NULL) {
		memset(buf, 0, bufSize * sizeof(float));
	}
}

void DelayReverb::resetParameters() {
	leftDelay = leftDelaySeconds * sampleRate;
	rightDelay = rightDelaySeconds * sampleRate;

	rampCount = 0;
	reverbLevel = 0;
	feedbackLevel = 0;
	feedbackLevelRampInc = targetFeedbackLevel / rampTarget;
	reverbLevelRampInc = targetReverbLevel / rampTarget;
}
