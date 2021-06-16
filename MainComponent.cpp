/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include <stdio.h>
#include "sqlite3.h"

//==============================================================================
MainComponent::MainComponent() : midiKeys(keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1400, 720);
	

	midiKeys.setAvailableRange(33, 81);
	midiKeys.setKeyWidth(50);
	midiKeys.setBlackNoteWidthProportion(.6);
	keyboardState.addListener(this);

	bitCrush.setSliderStyle(Slider::Rotary);
	bitCrush.setRange(0, 3, 1); //Bit crush range from 0 (full bit depth) to 3 (4 bit)
	bitCrush.setTextBoxStyle(Slider::TextEntryBoxPosition::NoTextBox, 1, 0, 0);
	bitCrush.addListener(this);

	bitCrushMode.setReadOnly(true);
	bitCrushMode.setCaretVisible(false);
	
	//bitCrushMode.setFont()
	bitCrushMode.setText("Bit Crush off", false);

	volumeSlider.setSliderStyle(Slider::LinearBarVertical);
	volumeSlider.setRange(0, 100, 1);
	volumeSlider.setSkewFactor(0.25, false);
	volumeSlider.addListener(this);

	waveSelect.addListener(this);

	


	//addAndMakeVisible(waveSelect);
	addAndMakeVisible(volumeSlider);
	addAndMakeVisible(midiKeys);
	addAndMakeVisible(bitCrush);
	addAndMakeVisible(bitCrushMode);


    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
	//Close database
	int rc = sqlite3_close(db);
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

	
	
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()





	playNote = false;
	currentSampleRate = sampleRate;
	volume = .05; //!!!!!!!!!!!static volume, will change later
	playFrequency = 440; //Initiate to A for now
	phase = 0; //NOTE maybe if running multiple waves at once, option to make them out of pahse by providing
				//individual phase variables per wave
	waveTableSize = 1024; //This seems arbitrary, bit depth or something? (Yes, 1024 = 2^10, which is a high resolution. Droping to 2,4,8,16 results in bit crush sounds)

	increment = (playFrequency * waveTableSize) / currentSampleRate; // If i need to complete 1 cycle of a wave 440 times (playFrequency)
															//and The waveTableSize represents one cycle, multiply the two.
															//Now, since we are bound by a clock cycle (sampleRate)
															//our "increment" (how much to move in the waveTable to achieve this) is the above divided by sampleRate
														// (SampleNumber * sample) / (rate One sample done per (second/instance)) = number of seconds/instances

	

	volumeSlider.setValue(100 * volume);//match volume slider to value

	
	startDB();

	
	
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    //bufferToFill.clearActiveBufferRegion();

	//Get the initail chanel pointers to send signal to
	float* const leftChannel = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
	float* const rightChannel = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);




	// fill a midi buffer with incoming messages from the midi input.
	MidiBuffer incomingMidi;
	//midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples);

	keyboardState.processNextMidiBuffer(incomingMidi, 0, bufferToFill.numSamples, true);

	




	Array<float> targetTable;

	if (bitCrush.getValue() == 0)
	{
		targetTable = waveTable;
		
	}
	else if (bitCrush.getValue() == 1)
	{
		targetTable = waveTable16Bit;
		
	}
	else if (bitCrush.getValue() == 2)
	{
		targetTable = waveTable8Bit;
		
	}
	else if (bitCrush.getValue() == 3)
	{
		targetTable = waveTable4Bit;
		
	}

	if (playNote)
	{
		for (int sample = 0; sample < bufferToFill.numSamples; sample++)
		{
			leftChannel[sample] = targetTable[(int)phase] * volume;
			rightChannel[sample] = targetTable[(int)phase] * volume;

			phase = fmod(phase + increment, waveTableSize); //move to next increment of samples...then modulus to wrap around

		}
	}
	

}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

	midiKeys.setBounds(0, getHeight()-200, getWidth(), 200);
	bitCrush.setBounds(0, 0, 250, 250);
	bitCrushMode.setBounds(0, 250, 250, 50);
	volumeSlider.setBounds(260, 50, 100, 200);
	waveSelect.setBounds(400, 50, 200, 50);
}


void MainComponent::sliderValueChanged(Slider* slider) 
{
	if (slider == &volumeSlider)
	{
		volume = volumeSlider.getValue() * .01; // 1/100 of value to be within range of 0 to 100
	}
	else if (slider == &bitCrush)
	{
		updateBitCrushMode(bitCrush.getValue());
	}
}

void MainComponent::updateBitCrushMode(double value)
{
	if (value == 0)
	{
		waveTableSize = 1024;
		bitCrushMode.setText("Bit Crush off");
	}
	else if (value == 1)
	{
		waveTableSize = 16;
		bitCrushMode.setText("16 Bit");
		
	}
	else if (value == 2)
	{
		waveTableSize = 8;
		bitCrushMode.setText("8 Bit");
	}
	else if (value == 3)
	{
		waveTableSize = 4;
		bitCrushMode.setText("4 Bit");
	}
	increment = (playFrequency * waveTableSize) / currentSampleRate;
	phase = 0;
}

void MainComponent::handleNoteOn(MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
	std::cout << midiNoteNumber;
	playFrequency = getMidiNoteInHertz(midiNoteNumber);
	//playFrequency = pow(2, ((midiNoteNumber - 69) / 12)) * 440;
	increment = (playFrequency * waveTableSize) / currentSampleRate;
	playNote = true;
}


void MainComponent::handleNoteOff(MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
	playNote = false;
}

void MainComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	waveSelect.setItemEnabled(currentWaveID, true);
	currentWaveID = waveSelect.getSelectedId();//used for switching when button pushed
	waveSelect.setItemEnabled(currentWaveID, false);

	//load wave tables
	loadWaveTables(currentWaveID - 1);//subtract 1 because id in combo can't be 0, so adjust for databases
}




void MainComponent::startDB()
{
	//sqlite3* db;
	char* zErrMsg = 0;
	int rc;
	rc = sqlite3_open(waveTableDB, &db);
	const char* sqlComm;

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		//PUT INIT DATABASE FUNTION THING HERE
		//USE demo vid's example of how they intialize waveTable, except we will place in a .db file with SQLite


	}
	else {
		fprintf(stderr, "Opened database successfully\n");
		//SET UP INTIAL AUDIO WAVE TABLE HERE

		//First, check if database is already created using EXISTS(subquerry)
		bool needToInit;
		std::string sqlCheck = "SELECT t1024 FROM wave_table_1024 WHERE name = 'Sine'";
		sqlComm = sqlCheck.c_str();
		sqlite3_exec(db, sqlComm, checkNewCallback, &needToInit, &zErrMsg);

		if (!needToInit)//if we need to set everything up...
		{
			initDB(); //call initDB(), which will create tables and fill with values
		}

		//Call loadWaveTables(0), which will load default sine wavetable
		loadWaveTables(0);

		//load up wave menu ComboBox
		sqlCheck = "SELECT * FROM wave_names";
		sqlComm = sqlCheck.c_str();
		sqlite3_exec(db, sqlComm, assignMenuCallback, &waveSelect, &zErrMsg);
		
		waveSelect.setSelectedId(1);
		waveSelect.setItemEnabled(1, false);
		currentWaveID = 1;//used for switching when button pushed
		addAndMakeVisible(waveSelect);
	}

	

}


int MainComponent::checkNewCallback(void* needToInit, int argc, char** argv, char** azColName)
{
	bool* setter = (bool*)needToInit;
	
		*setter = true;
	return 1; //returning 1 will stop any further calls, if ther are any
}

void MainComponent::initDB()
{
	std::string temp;//convert from string to char*
	const char* sqlComm;
	double waveVal;

	//=======    Sine   Square    Triangle     SawTooth

	char* zErrMsg = 0;
	int rc;
	//Create wave_name table
	std::string sql = "CREATE TABLE wave_names(waveID INT PRIMARY KEY NOT NULL, name TEXT NOT NULL);";
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);
	//populate wave_name
	sql = "INSERT INTO wave_names(waveID, name) VALUES (0, 'Sine'); INSERT INTO wave_names(waveID, name) VALUES (1, 'Square'); INSERT INTO wave_names(waveID, name) VALUES (2, 'Triangle'); INSERT INTO wave_names(waveID, name) VALUES (3, 'SawTooth');";
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);




	//Create wave_table_1024
	sql = "CREATE TABLE wave_table_1024(name TEXT PRIMARY KEY NOT NULL);";
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	sql = "INSERT INTO wave_table_1024(name) VALUES ('Sine');";
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	sql = "INSERT INTO wave_table_1024(name) VALUES ('Square');";
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	sql = "INSERT INTO wave_table_1024(name) VALUES ('Triangle');";
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_1024(name) VALUES ('SawTooth');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	//Populate Values
	for (int i = 0; i < 1024; i++)
	{
		sql = "ALTER TABLE wave_table_1024 ADD COLUMN t" + std::to_string(i) + " INT;";
		sqlComm = sql.c_str();
		//strcpy(sql, temp.c_str());
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Sine
		sql = "UPDATE wave_table_1024 SET t" + std::to_string(i) + std::string(" = ") + std::to_string(sin((2 * double_Pi * i) / 1024)) + " WHERE name = 'Sine';";
		sqlComm = sql.c_str();
		//strcpy(sql, temp.c_str());
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Square
		if (i < 1024 / 2)
		{
			sql = std::string("UPDATE wave_table_1024 SET t") + std::to_string(i) + " = 1 WHERE name = 'Square';";

		}
		else
		{
			sql = std::string("UPDATE wave_table_1024 SET t") + std::to_string(i) + " = 0 WHERE name = 'Square';";
		}
		
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);



		//Traingle
		sql = std::string("UPDATE wave_table_1024 SET t") + std::to_string(i) + std::string(" = ") + std::to_string(asin(cos((2 * double_Pi * i) / 1024))) + " WHERE name = 'Triangle';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//SawTooth
		sql = std::string("UPDATE wave_table_1024 SET t") + std::to_string(i) + std::string(" = ") + std::to_string((-atan(1 / (tan((2 * double_Pi * i) / 1024))))) + " WHERE name = 'SawTooth';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);
	}


	//Create wave_table_16
	 sql = "CREATE TABLE wave_table_16(name TEXT PRIMARY KEY NOT NULL);";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_16(name) VALUES ('Sine');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_16(name) VALUES ('Square');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_16(name) VALUES ('Triangle');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_16(name) VALUES ('SawTooth');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	//Populate Values
	for (int i = 0; i < 16; i++)
	{
		sql = "ALTER TABLE wave_table_16 ADD COLUMN t" + std::to_string(i) + " INT;";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Sine
		sql = "UPDATE wave_table_16 SET t" + std::to_string(i) + std::string(" = ") + std::to_string(sin((2 * double_Pi * i) / 16)) + " WHERE name = 'Sine';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Square
		if (i < 16 / 2)
		{
			sql = std::string("UPDATE wave_table_16 SET t") + std::to_string(i) + " = 1 WHERE name = 'Square';";
		}
		else
		{
			sql = std::string("UPDATE wave_table_16 SET t") + std::to_string(i) + " = 0 WHERE name = 'Square';";
		}

		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);



		//Traingle
		sql = std::string("UPDATE wave_table_16 SET t") + std::to_string(i) + std::string(" = ") + std::to_string(asin(cos((2 * double_Pi * i) / 16))) + " WHERE name = 'Triangle';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//SawTooth
		sql = std::string("UPDATE wave_table_16 SET t") + std::to_string(i) + std::string(" = ") + std::to_string((-atan(1 / (tan((2 * double_Pi * i) / 16))))) + " WHERE name = 'SawTooth';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);
	}


	//Create wave_table_8
	 sql = "CREATE TABLE wave_table_8(name TEXT PRIMARY KEY NOT NULL);";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_8(name) VALUES ('Sine');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_8(name) VALUES ('Square');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_8(name) VALUES ('Triangle');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_8(name) VALUES ('SawTooth');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	//Populate Values
	for (int i = 0; i < 8; i++)
	{
		sql = "ALTER TABLE wave_table_8 ADD COLUMN t" + std::to_string(i) + " INT;";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Sine
		sql = "UPDATE wave_table_8 SET t" + std::to_string(i) + std::string(" = ") + std::to_string(sin((2 * double_Pi * i) / 8)) + " WHERE name = 'Sine';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Square
		if (i < 8 / 2)
		{
			sql = std::string("UPDATE wave_table_8 SET t") + std::to_string(i) + " = 1 WHERE name = 'Square';";
		}
		else
		{
			sql = std::string("UPDATE wave_table_8 SET t") + std::to_string(i) + " = 0 WHERE name = 'Square';";
		}
		sqlComm = sql.c_str();
		//strcpy(sql, temp.c_str());
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);



		//Traingle
		sql = std::string("UPDATE wave_table_8 SET t") + std::to_string(i) + std::string(" = ") + std::to_string(asin(cos((2 * double_Pi * i) / 8))) + " WHERE name = 'Triangle';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//SawTooth
		sql = std::string("UPDATE wave_table_8 SET t") + std::to_string(i) + std::string(" = ") + std::to_string((-atan(1 / (tan((2 * double_Pi * i) / 8))))) + " WHERE name = 'SawTooth';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);
	}


	//Create wave_table_4
	 sql = "CREATE TABLE wave_table_4(name TEXT PRIMARY KEY NOT NULL);";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_4(name) VALUES ('Sine');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_4(name) VALUES ('Square');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_4(name) VALUES ('Triangle');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	 sql = "INSERT INTO wave_table_4(name) VALUES ('SawTooth');";
	 sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);

	//Populate Values
	for (int i = 0; i < 4; i++)
	{
		sql = "ALTER TABLE wave_table_4 ADD COLUMN t" + std::to_string(i) + " INT;";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Sine
		sql = "UPDATE wave_table_4 SET t" + std::to_string(i) + std::string(" = ") + std::to_string(sin((2 * double_Pi * i) / 4)) + " WHERE name = 'Sine';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//Square
		if (i < 4 / 2)
		{
			sql = std::string("UPDATE wave_table_4 SET t") + std::to_string(i) + " = 1 WHERE name = 'Square';";
		}
		else
		{
			sql = std::string("UPDATE wave_table_4 SET t") + std::to_string(i) + " = 0 WHERE name = 'Square';";
		}

		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);



		//Traingle
		sql = std::string("UPDATE wave_table_4 SET t") + std::to_string(i) + std::string(" = ") + std::to_string(asin(cos((2 * double_Pi * i) / 4))) + " WHERE name = 'Triangle';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);


		//SawTooth
		sql = std::string("UPDATE wave_table_4 SET t") + std::to_string(i) + std::string(" = ") + std::to_string((-atan(1 / (tan((2 * double_Pi * i) / 4))))) + " WHERE name = 'SawTooth';";
		//strcpy(sql, temp.c_str());
		sqlComm = sql.c_str();
		rc = sqlite3_exec(db, sqlComm, loadDBCallback, 0, &zErrMsg);
	}



}

int MainComponent::loadDBCallback(void* NotUsed, int argc, char** argv, char** azColName)
{

	return 0;
}

void MainComponent::loadWaveTables(int tableID)
{	
	char* zErrMsg = 0;
	int rc;
	//char* sql;
	std::string sql;
	const char* sqlComm;

	//Clear wavetables first
	waveTable.clear();
	waveTable16Bit.clear();
	waveTable8Bit.clear();
	waveTable4Bit.clear();

	//Grab everything, use callback function to handle loading
	sql = std::string("SELECT * FROM wave_table_1024 WHERE name = (SELECT name FROM wave_names WHERE waveID = ") + std::to_string(tableID) + std::string(");");
	//strcpy(sql, temp.c_str());
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadTableCallback, &waveTable, &zErrMsg);

	sql = std::string("SELECT * FROM wave_table_16 WHERE name = (SELECT name FROM wave_names WHERE waveID = ") + std::to_string(tableID) + std::string(");");
	//strcpy(sql, temp.c_str());
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadTableCallback, &waveTable16Bit, &zErrMsg);

	sql = std::string("SELECT * FROM wave_table_8 WHERE name = (SELECT name FROM wave_names WHERE waveID = ") + std::to_string(tableID) + std::string(");");
	//strcpy(sql, temp.c_str());
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadTableCallback, &waveTable8Bit, &zErrMsg);

	sql = std::string("SELECT * FROM wave_table_4 WHERE name = (SELECT name FROM wave_names WHERE waveID = ") + std::to_string(tableID) + std::string(");");
	//strcpy(sql, temp.c_str());
	sqlComm = sql.c_str();
	rc = sqlite3_exec(db, sqlComm, loadTableCallback, &waveTable4Bit, &zErrMsg);

	/*
	for (int i = 0; i < 1024; i++)
	{
		waveTable.insert(i, sin((2 * double_Pi * i) / waveTableSize));
	}
	
	for (int i = 0; i < 4; i++)
	{
		waveTable4Bit.insert(i, sin((2 * double_Pi * i) / waveTableSize));
	}
	
	for (int i = 0; i < 8; i++)
	{
		waveTable8Bit.insert(i, sin((2 * double_Pi * i) / waveTableSize));
	}
	
	for (int i = 0; i < 16; i++)
	{
		waveTable16Bit.insert(i, sin((2 * double_Pi * i) / waveTableSize));
	}
	*/
}
int MainComponent::loadTableCallback(void* waveTableTarget, int argc, char** argv, char** azColName)
{
	Array<float>* tempTable = (Array<float>*)waveTableTarget;
	
	//NOTE! remember SELECT * will also include column for name as well, so need to increment to end, but assign wavetable accordingly
	for (int i = 1; i < argc; i++)
	{
		tempTable->add(std::stof(argv[i]));
		//tempTable->add(.1);
	}
	
	return 0;
}

int MainComponent::assignMenuCallback(void* comboTarget, int argc, char** argv, char** azColName)
{
	ComboBox* tempBox = (ComboBox*)comboTarget;
	tempBox->addItem(argv[1], std::stoi(argv[0]) + 1); //plus 1 since combo box can't have id of 0
	return 0;
}

