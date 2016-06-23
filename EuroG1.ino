#include <digitalWriteFast.h>
#include <MIDI.h>

struct MySettings : public midi::DefaultSettings
{
	//static const bool UseRunningStatus = false;
	static const bool UseHandleNullVelocityNoteOnAsNoteOff = false;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);

//MIDI_CREATE_DEFAULT_INSTANCE();

unsigned long controlTimer = 0;

//
//	**ASSIGN CONTROLS TO MIDI CC**
//

int CC07MasterVolume = 4;	//	A4
int CC01LFOFilterMod = 8;
int CC16LFORate = 7;
int CC20LFOWave = 8;		//	D8
int CC74VCFCutoff = 5;
int CC71VCFRez = 6;
int CC82VCFEnvA = 9;
int CC83VCFEnvD = 10;
int CC28VCFEnvS = 11;
int CC29VCFEnvR = 12;
int CC81VCFEnvMod = 6;		//	A6
int CC76VCOWave = 1;
int CC04VCOWrap = 4;
int CC21VCORange = 2;
int CC93VCODetune = 3;
int CC73VCAEnvA = 13;
int CC75VCAEnvD = 14;
int CC31VCAEnvS = 15;
int CC72VCAEnvR = 16;

int CVIn = A7;

//	CONTROL VALUES

int CC07MasterVolumeValue = -1;
int CC01LFOFilterModValue = -1;
int CC16LFORateValue = -1;
bool CC20LFOWaveValue;
int CC74VCFCutoffValue = -1;
int CC71VCFRezValue = -1;
int CC82VCFEnvAValue = -1;
int CC83VCFEnvDValue = -1;
int CC28VCFEnvSValue = -1;
int CC29VCFEnvRValue = -1;
int CC81VCFEnvModValue = -1;
int CC76VCOWaveValue = -1;
int CC04VCOWrapValue = -1;
int CC21VCORangeValue = -1;
int CC93VCODetuneValue = -1;
int CC73VCAEnvAValue = -1;
int CC75VCAEnvDValue = -1;
int CC31VCAEnvSValue = -1;
int CC72VCAEnvRValue = -1;

int LastCC07MasterVolumeValue = 999;
int LastCC01LFOFilterModValue = 999;
int LastCC16LFORateValue = 999;
bool LastCC20LFOWaveValue;
int LastCC74VCFCutoffValue = 999;
int LastCC71VCFRezValue = 999;
int LastCC82VCFEnvAValue = 999;
int LastCC83VCFEnvDValue = 999;
int LastCC28VCFEnvSValue = 999;
int LastCC29VCFEnvRValue = 999;
int LastCC81VCFEnvModValue = 999;
int LastCC76VCOWaveValue = 999;
int LastCC04VCOWrapValue = 999;
int LastCC21VCORangeValue = 999;
int LastCC93VCODetuneValue = 999;
int LastCC73VCAEnvAValue = 999;
int LastCC75VCAEnvDValue = 999;
int LastCC31VCAEnvSValue = 999;
int LastCC72VCAEnvRValue = 999;

int r0 = 0;      //value of select pin at the 4051 (s0)
int r1 = 0;      //value of select pin at the 4051 (s1)
int r2 = 0;      //value of select pin at the 4051 (s2)
int count = 0;   //which y pin we are selecting
int mValue[17];
int trig = 9;		//	Pin 9 for gate input
bool triggered = false;
bool checkTrig;
int note = 0;
int sentNote = 0;
int ADCValue;
float voltage;
float floatNote;
double Vcc;
int dSentNote;
int dNote;
int velocity;
int noteTransposedM;
int noteTransposed;
int noteM = 0;
int sentNoteM = 0;

void setup()
{
	MIDI.begin(MIDI_CHANNEL_OMNI);
	MIDI.turnThruOff();
	pinMode(CC20LFOWave, INPUT_PULLUP);
	pinMode(9, INPUT);

	//	Multiplexer Select Pins
	pinMode(10, OUTPUT);    // s0
	pinMode(11, OUTPUT);    // s1
	pinMode(12, OUTPUT);    // s2

	//	Kill any spurious notes on startup.
	allNotesOff();
	delay(100);
	readControls();
	//delay(3000);
	//allNotesOff();
}

void loop()
{
	//	Only check controls every 5ms for less MIDI latency.
	if (millis() - controlTimer > 5)

	{
		controlTimer = millis();
		readControls();
	}

	//
	//	Handle CV/GATE inputs
	//
	checkTrig = digitalRead(trig);
	if (checkTrig)		//	Key pressed.
	{
		//	CV is 0-5V 1V/Oct
		ADCValue = analogRead(CVIn);
		voltage = ((ADCValue / 1024.0) * 5);
		note = ((voltage / .083) + .5);
		noteTransposed = note - 12; // note+12 fits MIDI note number to CV range, but shift it two octaves to have better bass CV range.
		if (noteTransposed != sentNote || triggered == false)
		{		//	If this is a new note OR a first note...
			MIDI.sendNoteOff(sentNote, 0, 1);			//	Stop previous note in case it's still playing.
			if ((noteTransposed >= 0) && (noteTransposed <= 48))	//	Don't really play note if its out of range. Note that this changes depending on how much it's transposed. Un-transposed playable range is 0-61.
			{
				MIDI.sendNoteOn(noteTransposed, 127, 1);
			}
			sentNote = noteTransposed;
			triggered = true;
		}
	}

	if (!checkTrig && triggered)		//	First Key up
	{
		MIDI.sendNoteOff(sentNote, 0, 1);
		triggered = false;
	}
	//
	//	Handle MIDI Note-Ons and Note-Offs. Treat MIDI as monophonic.
	//
	if (MIDI.read()) 
	{					
		noteM = MIDI.getData1();
		noteTransposedM = noteM-24; // Fit range of typical keyboard better.
		velocity = MIDI.getData2();

		if (MIDI.getType() == midi::NoteOn)	//	Only pass NoteOn/NoteOff to G1
		{
			if (velocity == 0)
			{
				MIDI.sendNoteOff(noteTransposedM, 0, 1);
			}

			else
			{
				if ((noteTransposedM >= 0) && (noteTransposedM <= 84))
				{
					MIDI.sendNoteOff(sentNoteM, 0, 1);
					MIDI.sendNoteOn(noteTransposedM, 127, 1);
					sentNoteM = noteTransposedM;
				}
			}
		}

		if (MIDI.getType() == midi::NoteOff)
		{
			MIDI.sendNoteOff(noteTransposedM, 0, 1);
		}

		if (MIDI.getType() == midi::ControlChange)
		{
			MIDI.sendControlChange(MIDI.getData1(), MIDI.getData2(),1);
		}

		if (MIDI.getType() == midi::Stop) { allNotesOff(); } //	Stop dead notes if a sequencer stops
	}
}

void readControls()
{
	//	Read all controls, and if a change since last read send MIDI CC to G1

	for (count = 0; count <= 7; count++)
	{
		// Select the bit

		r0 = bitRead(count, 0);
		r1 = bitRead(count, 1);
		r2 = bitRead(count, 2);
		digitalWriteFast(10, r0);
		digitalWriteFast(11, r1);
		digitalWriteFast(12, r2);

		//	Fill array(1-16) with values from multiplexer
		mValue[count + 1] = analogRead(A1) / 8;
		mValue[count + 9] = analogRead(A0) / 8;
	}

	// Assign value to Controller

	CC76VCOWaveValue = mValue[1];
	CC21VCORangeValue = mValue[2];
	CC93VCODetuneValue = mValue[3];
	CC04VCOWrapValue = mValue[4];
	CC74VCFCutoffValue = mValue[5];
	CC71VCFRezValue = mValue[6]/1.5;	//	Reduce resonance a bit more
	CC16LFORateValue = mValue[7];
	CC01LFOFilterModValue = mValue[8];
	CC82VCFEnvAValue = mValue[9];
	CC83VCFEnvDValue = mValue[10];
	CC28VCFEnvSValue = mValue[11];
	CC29VCFEnvRValue = mValue[12];
	CC73VCAEnvAValue = mValue[13];
	CC75VCAEnvDValue = mValue[14];
	CC31VCAEnvSValue = mValue[15];
	CC72VCAEnvRValue = mValue[16];

	int value;

	//	Master Volume
	CC07MasterVolumeValue = analogRead(CC07MasterVolume) / 8;
	if (CC07MasterVolumeValue != LastCC07MasterVolumeValue)
	{
		LastCC07MasterVolumeValue = CC07MasterVolumeValue;
		//int value = float ((CC07MasterVolumeValue*16)+.5);	//	Map 10 bit value to 14 bitvalue and round.
		//MIDI.sendControlChange(7, highByte(value), 1);		//	Don't need this because the G1 doesn't use
		//MIDI.sendControlChange(39, lowByte(value), 1);		//	high-resolution continuous controllers. Maybe later?
		MIDI.sendControlChange(07, CC07MasterVolumeValue, 1);
	}

	//	VCF Envelope
	CC81VCFEnvModValue = analogRead(CC81VCFEnvMod) / 8;
	if (CC81VCFEnvModValue != LastCC81VCFEnvModValue)
	{
		LastCC81VCFEnvModValue = CC81VCFEnvModValue;
		MIDI.sendControlChange(81, CC81VCFEnvModValue, 1);
	}

	//	VCF Wave
	CC20LFOWaveValue = digitalRead(CC20LFOWave);
	if (CC20LFOWaveValue != LastCC20LFOWaveValue)
	{
		LastCC20LFOWaveValue = CC20LFOWaveValue;
		if (CC20LFOWaveValue == 1)							//	Switches are 0 or 127
		{
			value = 127;
		}
		else
		{
			value = 0;
		}
		MIDI.sendControlChange(20, value, 1);
	}

	//	VCO Wave
	if (CC76VCOWaveValue != LastCC76VCOWaveValue)
	{
		LastCC76VCOWaveValue = CC76VCOWaveValue;
		MIDI.sendControlChange(76, CC76VCOWaveValue, 1);
	}

	//	VCO Range
	if (CC21VCORangeValue != LastCC21VCORangeValue)
	{
		LastCC21VCORangeValue = CC21VCORangeValue;
		MIDI.sendControlChange(21, CC21VCORangeValue, 1);
	}

	//	VCO Detune
	if (CC93VCODetuneValue != LastCC93VCODetuneValue)
	{
		LastCC93VCODetuneValue = CC93VCODetuneValue;
		MIDI.sendControlChange(93, CC93VCODetuneValue, 1);
	}

	//	VCO Wrap
	if (CC04VCOWrapValue != LastCC04VCOWrapValue)
	{
		LastCC04VCOWrapValue = CC04VCOWrapValue;
		MIDI.sendControlChange(04, CC04VCOWrapValue, 1);
	}

	//	VCF Cutoff
	if (CC74VCFCutoffValue != LastCC74VCFCutoffValue)
	{
		LastCC74VCFCutoffValue = CC74VCFCutoffValue;
		MIDI.sendControlChange(74, CC74VCFCutoffValue, 1);
	}

	//	VCF Resonance
	if (CC71VCFRezValue != LastCC71VCFRezValue)
	{
		LastCC71VCFRezValue = CC71VCFRezValue;
		MIDI.sendControlChange(71, CC71VCFRezValue, 1);
	}

	//	LFO Rate
	if (CC16LFORateValue != LastCC16LFORateValue)
	{
		LastCC16LFORateValue = CC16LFORateValue;
		MIDI.sendControlChange(16, CC16LFORateValue, 1);
	}

	//	LFO Filter Modulation
	if (CC01LFOFilterModValue != LastCC01LFOFilterModValue)
	{
		LastCC01LFOFilterModValue = CC01LFOFilterModValue;
		MIDI.sendControlChange(01, CC01LFOFilterModValue, 1);
	}

	//	VCF Envelope Attack
	if (CC82VCFEnvAValue != LastCC82VCFEnvAValue)
	{
		LastCC82VCFEnvAValue = CC82VCFEnvAValue;
		MIDI.sendControlChange(82, CC82VCFEnvAValue, 1);
	}

	//	VCF Envelope Delay
	if (CC83VCFEnvDValue != LastCC83VCFEnvDValue)
	{
		LastCC83VCFEnvDValue = CC83VCFEnvDValue;
		MIDI.sendControlChange(83, CC83VCFEnvDValue, 1);
	}

	//	VCF Envelope Sustain
	if (CC28VCFEnvSValue != LastCC28VCFEnvSValue)
	{
		LastCC28VCFEnvSValue = CC28VCFEnvSValue;
		MIDI.sendControlChange(28, CC28VCFEnvSValue, 1);
	}

	//	VCF Envelope Release
	if (CC29VCFEnvRValue != LastCC29VCFEnvRValue)
	{
		LastCC29VCFEnvRValue = CC29VCFEnvRValue;
		MIDI.sendControlChange(29, CC29VCFEnvRValue, 1);
	}

	//	VCA Envelope Attack
	if (CC73VCAEnvAValue != LastCC73VCAEnvAValue)
	{
		LastCC73VCAEnvAValue = CC73VCAEnvAValue;
		MIDI.sendControlChange(73, CC73VCAEnvAValue, 1);
	}

	//	VCA Envelope Decay
	if (CC75VCAEnvDValue != LastCC75VCAEnvDValue)
	{
		LastCC75VCAEnvDValue = CC75VCAEnvDValue;
		MIDI.sendControlChange(75, CC75VCAEnvDValue, 1);
	}

	//	VCA Envelope Sustain
	if (CC31VCAEnvSValue != LastCC31VCAEnvSValue)
	{
		LastCC31VCAEnvSValue = CC31VCAEnvSValue;
		MIDI.sendControlChange(31, CC31VCAEnvSValue, 1);
	}

	//	VCA Envelope Release
	if (CC72VCAEnvRValue != LastCC72VCAEnvRValue)
	{
		LastCC72VCAEnvRValue = CC72VCAEnvRValue;
		MIDI.sendControlChange(72, CC72VCAEnvRValue, 1);	//	Sometimes glitch here: CC value 0 at full 5V. All controls stop at 0V. Fixed with slower update speed.
	}

	/*
	//	Controller Template
	if (XXXValue != LastXXXValue)
	{
	LastXXXValue = XXXValue;
	MIDI.sendControlChange(00, XXXValue / 8, 1);
	}
	*/
}

long readVcc()
{
	long result;
	// Read 1.1V reference against AVcc
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	delay(2); 	// Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Convert
	while (bit_is_set(ADCSRA, ADSC));
	result = ADCL;
	result |= ADCH << 8;
	result = 1125300L / result;	// Back-calculate AVcc in mV
	return result;
}

void allNotesOff()
{
	for (int offNote = 0; offNote <= 127; offNote++)
	{
		MIDI.sendNoteOff(offNote, 0, 1);
		//delay(10);
	}
}