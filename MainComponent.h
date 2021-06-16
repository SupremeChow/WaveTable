/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "sqlite3.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public AudioAppComponent, public Slider::Listener, public MidiKeyboardStateListener, public MidiMessage, public ComboBox::Listener
{
public:

    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

	//===========================================================
	virtual void sliderValueChanged(Slider* slider);

	void updateBitCrushMode(double value);
	virtual void handleNoteOn(MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity);
	virtual void handleNoteOff(MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity);
	
	//hold off on below, may not need
	virtual void comboBoxChanged(ComboBox* comboBoxThatHasChanged);

private:
    //================================WaveTable Stuff==============================================
    // Your private member variables go here...
	Array<float> waveTable; //The wave table being referenced
							//ADD other tables if eventually working with other load ins
							//do later
	Array<float> waveTable16Bit;
	Array<float> waveTable8Bit;
	Array<float> waveTable4Bit;
	double waveTableSize; //Size of the table. IN the demo, this is used for initializing i think.
							//Could be related to sample rate??
	double playFrequency; //Frequency in which to play the table back. 
							//!! Later, should be modular and change by keystroke or something
							//!! for now, just follow demo and use slider
	double phase;		//When incrementing through table, this holds position in sine wav's phase
	double increment;	//HOw much to increment through data. Buffer size i think

	double volume; //Control amplitude of waves
	//Slider frequencySlider;
	Slider volumeSlider;
	Slider bitCrush;
	TextEditor bitCrushMode;
	MidiKeyboardState keyboardState;
	MidiKeyboardComponent midiKeys; //how we will control notes

	ComboBox waveSelect;
	int currentWaveID;

	bool playNote;

	double currentSampleRate; //need for tracking outside prepareToPlay()



	//==================SQLite stuff=====================//
	sqlite3* db; //Our database connection. When connecting to a database file, this is passed as an argument to be assgined an address

	void startDB();//start database for checking and such


	const char*  waveTableDB = "waveTable.db"; //Defualt name to database file we will connect to (const char*, not String or string

	


	//------SQL querries------

	std::string menuSelectLoad; //Querry for loading main menu for switching tables
	std::string tableNameTarget; //String for holding the name of the wavetable to load, taken from menuSelectLoad value

	//The following querries will load from the 4 main tables. Will use String tableNameTarget as variable in querry.
	std::string loadWave;// Load sine wavetables
	std::string loadWave16;
	std::string loadWave8;
	std::string loadWave4;


	static int checkNewCallback(void* NotUsed, int argc, char** argv, char** azColName); //for checking if new DB
	static int loadDBCallback(void* NotUsed, int argc, char** argv, char** azColName);//when creating db, sort of dummy callback (could output successful insert notes here)
	void initDB();//setup wavetable db when new one is created

	void loadWaveTables(int tableID);// Used for loading and switching out waveTables

	static int loadTableCallback(void* waveTableTarget, int argc, char** argv, char** azColName); //Callback function for handling loading wavetables from querries
	
	static int assignMenuCallback(void* comboTarget, int argc, char** argv, char** azColName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
