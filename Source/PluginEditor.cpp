/*
Copyright (C) 2006-2011 Nasca Octavian Paul
Author: Nasca Octavian Paul

Copyright (C) 2017 Xenakios

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (version 2) for more details.

You should have received a copy of the GNU General Public License (version 2)
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>

extern String g_plugintitle;

//==============================================================================
PaulstretchpluginAudioProcessorEditor::PaulstretchpluginAudioProcessorEditor(PaulstretchpluginAudioProcessor& p)
	: AudioProcessorEditor(&p),
	m_wavecomponent(p.m_afm,p.m_thumb.get()),
	processor(p), m_perfmeter(&p)

{
	addAndMakeVisible(&m_perfmeter);
	
	addAndMakeVisible(&m_import_button);
	m_import_button.setButtonText("Import file...");
	m_import_button.onClick = [this]() { chooseFile(); };
	
	addAndMakeVisible(&m_settings_button);
	m_settings_button.setButtonText("Settings...");
	m_settings_button.onClick = [this]() { showSettingsMenu(); };
	
	addAndMakeVisible(&m_info_label);
	m_info_label.setJustificationType(Justification::centredRight);

	m_wavecomponent.GetFileCallback = [this]() { return processor.getAudioFile(); };
	addAndMakeVisible(&m_wavecomponent);
	const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		AudioProcessorParameterWithID* parid = dynamic_cast<AudioProcessorParameterWithID*>(pars[i]);
		jassert(parid);
		bool notifyonlyonrelease = false;
		if (parid->paramID.startsWith("fftsize") || parid->paramID.startsWith("numoutchans") 
			|| parid->paramID.startsWith("numinchans"))
				notifyonlyonrelease = true;
		m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[i],notifyonlyonrelease));
		int group_id = -1;
		if (i == cpi_harmonicsbw || i == cpi_harmonicsfreq || i == cpi_harmonicsgauss || i == cpi_numharmonics)
			group_id = 0;
		if (i == cpi_octavesm2 || i == cpi_octavesm1 || i == cpi_octaves0 || i == cpi_octaves1 || i == cpi_octaves15 ||
			i == cpi_octaves2)
			group_id = 4;
		if (i == cpi_tonalvsnoisebw || i == cpi_tonalvsnoisepreserve)
			group_id = 1;
		if (i == cpi_filter_low || i == cpi_filter_high)
			group_id = 6;
		if (i == cpi_compress)
			group_id = 7;
		if (i == cpi_spreadamount)
			group_id = 5;
		if (i == cpi_frequencyshift)
			group_id = 2;
		if (i == cpi_pitchshift)
			group_id = 3;
		m_parcomps.back()->m_group_id = group_id;
		addAndMakeVisible(m_parcomps.back().get());
	}
	
	//addAndMakeVisible(&m_specvis);
	addAndMakeVisible(&m_zs);
	m_zs.RangeChanged = [this](Range<double> r)
	{
		m_wavecomponent.setViewRange(r);
		processor.m_wave_view_range = r;
	};
	m_zs.setRange(processor.m_wave_view_range, true);
	setSize (1000, 30+(pars.size()/2)*25+200+15);
	m_wavecomponent.TimeSelectionChangedCallback = [this](Range<double> range, int which)
	{
		*processor.getFloatParameter(cpi_soundstart) = range.getStart();
		*processor.getFloatParameter(cpi_soundend) = range.getEnd();
	};
	m_wavecomponent.CursorPosCallback = [this]()
	{
		return processor.getStretchSource()->getInfilePositionPercent();
	};
	m_wavecomponent.SeekCallback = [this](double pos)
	{
		if (processor.getStretchSource()->getPlayRange().contains(pos))
			processor.getStretchSource()->seekPercent(pos);
	};
	m_wavecomponent.ShowFileCacheRange = true;
	m_spec_order_ed.setSource(processor.getStretchSource());
	addAndMakeVisible(&m_spec_order_ed);
	m_spec_order_ed.ModuleSelectedCallback = [this](int id)
	{
		for (int i = 0; i < m_parcomps.size(); ++i)
		{
			if (m_parcomps[i]->m_group_id == id)
				m_parcomps[i]->setHighLighted(true);
			else
				m_parcomps[i]->setHighLighted(false);
		}
	};
	m_spec_order_ed.ModuleOrderOrEnabledChangedCallback = [this]()
	{
		processor.setDirty();
	};
	startTimer(1, 100);
	startTimer(2, 1000);
	startTimer(3, 200);
	m_wavecomponent.startTimer(100);
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
}

void PaulstretchpluginAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colours::darkgrey);
}

void PaulstretchpluginAudioProcessorEditor::resized()
{
	m_import_button.setBounds(1, 1, 60, 24);
	m_import_button.changeWidthToFitText();
	m_settings_button.setBounds(m_import_button.getRight() + 1, 1, 60, 24);
	m_settings_button.changeWidthToFitText();
	m_perfmeter.setBounds(m_settings_button.getRight() + 1, 1, 150, 24);
	m_info_label.setBounds(m_perfmeter.getRight() + 1, m_settings_button.getY(),
		getWidth()- m_perfmeter.getRight()-1, 24);
	int w = getWidth();
	int xoffs = 1;
	int yoffs = 30;
	int div = w / 5;
	//std::vector<std::vector<int>> layout;
	//layout.emplace_back(cpi_capture_enabled,	cpi_passthrough,	cpi_pause_enabled,	cpi_freeze);
	//layout.emplace_back(cpi_main_volume,		cpi_num_inchans,	cpi_num_outchans);
	m_parcomps[cpi_capture_enabled]->setBounds(xoffs, yoffs, div-1, 24);
	//xoffs += div;
	//m_parcomps[cpi_max_capture_len]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_passthrough]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_pause_enabled]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs += div;
	m_parcomps[cpi_freeze]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_bypass_stretch]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	div = w / 3;
	m_parcomps[cpi_main_volume]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs += div;
	m_parcomps[cpi_num_inchans]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_num_outchans]->setBounds(xoffs, yoffs, div-1, 24);
	div = w / 2;
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_fftsize]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_stretchamount]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_pitchshift]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_frequencyshift]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_octavesm2]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_octavesm1]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_octaves0]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_octaves1]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_octaves15]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_octaves2]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_numharmonics]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_harmonicsfreq]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_harmonicsbw]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_harmonicsgauss]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_spreadamount]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_compress]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_tonalvsnoisebw]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_tonalvsnoisepreserve]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	// filter here
	m_parcomps[cpi_filter_low]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_filter_high]->setBounds(xoffs, yoffs, div - 1, 24);
	
	xoffs = 1;
	yoffs += 25;
	
	m_parcomps[cpi_loopxfadelen]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_onsetdetection]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_soundstart]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_soundend]->setBounds(xoffs, yoffs, div - 1, 24);
#ifdef SOUNDRANGE_OFFSET_ENABLED
	yoffs += 25;
	xoffs = 1;
	m_parcomps[cpi_playrangeoffset]->setBounds(xoffs, yoffs, getWidth() - 2, 24);
#endif
	yoffs += 25;
	int remain_h = getHeight() - 1 - yoffs -15;
	m_spec_order_ed.setBounds(1, yoffs, getWidth() - 2, remain_h / 5 * 1);
	m_wavecomponent.setBounds(1, m_spec_order_ed.getBottom()+1, getWidth()-2, remain_h/5*4);
	m_zs.setBounds(1, m_wavecomponent.getBottom(), getWidth() - 2, 16);
	//m_specvis.setBounds(1, yoffs, getWidth() - 2, getHeight() - 1 - yoffs);
}

void PaulstretchpluginAudioProcessorEditor::timerCallback(int id)
{
	if (id == 1)
	{
		for (auto& e : m_parcomps)
			e->updateComponent();
		if (processor.isRecordingEnabled())
		{
			m_wavecomponent.setRecordingPosition(processor.getRecordingPositionPercent());
		} else
			m_wavecomponent.setRecordingPosition(-1.0);
		String infotext; 
		if (processor.m_show_technical_info)
		{
            infotext += String(processor.m_prepare_count)+" ";
            infotext += String(processor.getStretchSource()->m_param_change_count);
			infotext += " param changes ";
		}
		infotext += m_last_err + " [FFT size " +
			String(processor.getStretchSource()->getFFTSize())+"]";
		double outlen = processor.getStretchSource()->getOutputDurationSecondsForRange(processor.getStretchSource()->getPlayRange(), processor.getStretchSource()->getFFTSize());
		infotext += " [Output length " + secondsToString2(outlen)+"]";
		if (processor.m_abnormal_output_samples > 0)
			infotext += " " + String(processor.m_abnormal_output_samples) + " invalid sample values";
		if (processor.isNonRealtime())
			infotext += " (offline rendering)";
		if (processor.m_playposinfo.isPlaying)
			infotext += " "+String(processor.m_playposinfo.timeInSeconds,1);
		if (processor.m_show_technical_info)
			infotext += " " + String(m_wavecomponent.m_image_init_count) + " " + String(m_wavecomponent.m_image_update_count);
		m_info_label.setText(infotext, dontSendNotification);
		m_perfmeter.repaint();
	}
	if (id == 2)
	{
		m_wavecomponent.setTimeSelection(processor.getTimeSelection());
		if (processor.m_state_dirty)
		{
			m_spec_order_ed.setSource(processor.getStretchSource());
			processor.m_state_dirty = false;
		}
	}
	if (id == 3)
	{
		//m_specvis.setState(processor.getStretchSource()->getProcessParameters(), processor.getStretchSource()->getFFTSize() / 2,
		//	processor.getSampleRate());
	}
}

bool PaulstretchpluginAudioProcessorEditor::isInterestedInFileDrag(const StringArray & files)
{
	if (files.size() == 0)
		return false;
	File f(files[0]);
	String extension = f.getFileExtension().toLowerCase();
	if (processor.m_afm->getWildcardForAllFormats().containsIgnoreCase(extension))
		return true;
	return false;

}

void PaulstretchpluginAudioProcessorEditor::filesDropped(const StringArray & files, int x, int y)
{
	if (files.size() > 0)
	{
		File f(files[0]);
		processor.setAudioFile(f);
		toFront(true);
	}
}

void PaulstretchpluginAudioProcessorEditor::chooseFile()
{
	String initiallocfn = processor.m_propsfile->m_props_file->getValue("importfilefolder",
                                                File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
    File initialloc(initiallocfn);
	String filterstring = processor.m_afm->getWildcardForAllFormats();
	FileChooser myChooser("Please select audio file...",
		initialloc,
		filterstring,true);
	if (myChooser.browseForFileToOpen())
	{
        File resu = myChooser.getResult();
        String pathname = resu.getFullPathName();
        if (pathname.startsWith("/localhost"))
        {
            pathname = pathname.substring(10);
            resu = File(pathname);
        }
        processor.m_propsfile->m_props_file->setValue("importfilefolder", resu.getParentDirectory().getFullPathName());
        m_last_err = processor.setAudioFile(resu);
	}
}

void PaulstretchpluginAudioProcessorEditor::showSettingsMenu()
{
	PopupMenu menu;
	menu.addItem(4, "Reset parameters", true, false);
	menu.addItem(5, "Load file with plugin state", true, processor.m_load_file_with_state);
	menu.addItem(1, "Play when host transport running", true, processor.m_play_when_host_plays);
	menu.addItem(2, "Capture when host transport running", true, processor.m_capture_when_host_plays);
	
	int capturelen = *processor.getFloatParameter(cpi_max_capture_len);
	PopupMenu capturelenmenu;
	std::vector<int> capturelens{ 2,5,10,30,60,120 };
	for (int i=0;i<capturelens.size();++i)
		capturelenmenu.addItem(200+i, String(capturelens[i])+" seconds", true, capturelen == capturelens[i]);
	menu.addSubMenu("Capture buffer length", capturelenmenu);
	menu.addItem(3, "About...", true, false);
#ifdef JUCE_DEBUG
	menu.addItem(6, "Dump preset to clipboard", true, false);
#endif
	menu.addItem(7, "Show technical info", true, processor.m_show_technical_info);
	int r = menu.show();
	if (r >= 200 && r < 210)
	{
		int caplen = capturelens[r - 200];
		*processor.getFloatParameter(cpi_max_capture_len) = (float)caplen;
	}
	if (r == 1)
	{
		toggleBool(processor.m_play_when_host_plays);
	}
	if (r == 2)
	{
		toggleBool(processor.m_capture_when_host_plays);
	}
	if (r == 4)
	{
		processor.resetParameters();
	}
	if (r == 5)
	{
		toggleBool(processor.m_load_file_with_state);
	}
	if (r == 3)
	{
		String fftlib = fftwf_version;
		String juceversiontxt = String("JUCE ") + String(JUCE_MAJOR_VERSION) + "." + String(JUCE_MINOR_VERSION);
		String title = g_plugintitle;
#ifdef JUCE_DEBUG
		title += " (DEBUG)";
#endif
		AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
			title,
			"Plugin for extreme time stretching and other sound processing\nBuilt on " + String(__DATE__) + " " + String(__TIME__) + "\n"
			"Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania\n"
			"(C) 2017-2018 Xenakios\n\n"
			"Using " + fftlib + " for FFT\n\n"
			+ juceversiontxt + " (c) Roli. Used under the GPL license.\n\n"
			"GPL licensed source code for this plugin at : https://bitbucket.org/xenakios/paulstretchplugin/overview\n"
			, "OK",
			this);

	}
    
	if (r == 6)
	{
		ValueTree tree = processor.getStateTree(true,true);
		MemoryBlock destData;
		MemoryOutputStream stream(destData, true);
		tree.writeToStream(stream);
		String txt = Base64::toBase64(destData.getData(), destData.getSize());
		SystemClipboard::copyTextToClipboard(txt);
	}
	if (r == 7)
	{
		toggleBool(processor.m_show_technical_info);
		processor.m_propsfile->m_props_file->setValue("showtechnicalinfo", processor.m_show_technical_info);
	}
}

WaveformComponent::WaveformComponent(AudioFormatManager* afm, AudioThumbnail* thumb)
{
	TimeSelectionChangedCallback = [](Range<double>, int) {};
	if (m_use_opengl == true)
		m_ogl.attachTo(*this);
	m_thumbnail = thumb;
	m_thumbnail->addChangeListener(this);
	setOpaque(true);
}

WaveformComponent::~WaveformComponent()
{
	if (m_use_opengl == true)
		m_ogl.detach();
	m_thumbnail->removeChangeListener(this);
}

void WaveformComponent::changeListenerCallback(ChangeBroadcaster * /*cb*/)
{
	jassert(MessageManager::getInstance()->isThisTheMessageThread());
	m_image_dirty = true;
	//repaint();
}

void WaveformComponent::updateCachedImage()
{
	Graphics tempg(m_waveimage);
	tempg.fillAll(Colours::black);
	tempg.setColour(Colours::darkgrey);
	double thumblen = m_thumbnail->getTotalLength();
	m_thumbnail->drawChannels(tempg, { 0,0,getWidth(),getHeight() - m_topmargin },
		thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
	m_image_dirty = false;
	++m_image_update_count;
}

void WaveformComponent::paint(Graphics & g)
{
	jassert(GetFileCallback);
	//Logger::writeToLog("Waveform component paint");
	g.fillAll(Colours::black);
	g.setColour(Colours::darkgrey);
	g.fillRect(0, 0, getWidth(), m_topmargin);
	if (m_thumbnail == nullptr || m_thumbnail->getTotalLength() < 0.01)
	{
		g.setColour(Colours::aqua.darker());
		g.drawText("No file loaded", 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
		return;
	}
	g.setColour(Colours::lightslategrey);
	double thumblen = m_thumbnail->getTotalLength();
	double tick_interval = 1.0;
	if (thumblen > 60.0)
		tick_interval = 5.0;
	for (double secs = 0.0; secs < thumblen; secs += tick_interval)
	{
		float tickxcor = (float)jmap<double>(secs,
			thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 0.0f, (float)getWidth());
		g.drawLine(tickxcor, 0.0, tickxcor, (float)m_topmargin, 1.0f);
	}
	bool m_use_cached_image = true;
	if (m_use_cached_image == true)
	{
		if (m_image_dirty == true || m_waveimage.getWidth() != getWidth()
			|| m_waveimage.getHeight() != getHeight() - m_topmargin)
		{
			if (m_waveimage.getWidth() != getWidth()
				|| m_waveimage.getHeight() != getHeight() - m_topmargin)
			{
				m_waveimage = Image(Image::ARGB, getWidth(), getHeight() - m_topmargin, true);
				++m_image_init_count;
			}
			updateCachedImage();
		}
		g.drawImage(m_waveimage, 0, m_topmargin, getWidth(), getHeight() - m_topmargin, 0, 0, getWidth(), getHeight() - m_topmargin);

	}
	else
	{
		g.setColour(Colours::darkgrey);
		m_thumbnail->drawChannels(g, { 0,m_topmargin,getWidth(),getHeight() - m_topmargin },
			thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
	}

	g.setColour(Colours::white.withAlpha(0.5f));
	double sel_len = m_time_sel_end - m_time_sel_start;
	//if (sel_len > 0.0 && sel_len < 1.0)
	{
		int xcorleft = normalizedToViewX<int>(m_time_sel_start); 
		int xcorright = normalizedToViewX<int>(m_time_sel_end);
		g.fillRect(xcorleft, m_topmargin, xcorright - xcorleft, getHeight() - m_topmargin);
	}
	if (m_file_cached.first.getLength() > 0.0 &&
		(bool)ShowFileCacheRange.getValue())
	{
		g.setColour(Colours::red.withAlpha(0.2f));
		int xcorleft = (int)jmap<double>(m_file_cached.first.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		int xcorright = (int)jmap<double>(m_file_cached.first.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		g.fillRect(xcorleft, 0, xcorright - xcorleft, m_topmargin / 2);
		xcorleft = (int)jmap<double>(m_file_cached.second.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		xcorright = (int)jmap<double>(m_file_cached.second.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		if (xcorright - xcorleft>0)
		{
			g.setColour(Colours::blue.withAlpha(0.2f));
			g.fillRect(xcorleft, m_topmargin / 2, xcorright - xcorleft, m_topmargin / 2);
		}
	}

	g.setColour(Colours::white);
	if (CursorPosCallback)
	{
		g.fillRect(normalizedToViewX<int>(CursorPosCallback()), m_topmargin, 1, getHeight() - m_topmargin);
	}
	if (m_rec_pos >= 0.0)
	{
		g.setColour(Colours::lightpink);
		g.fillRect(normalizedToViewX<int>(m_rec_pos), m_topmargin, 1, getHeight() - m_topmargin);
	}
	g.setColour(Colours::aqua);
	g.drawText(GetFileCallback().getFileName(), 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
	g.drawText(secondsToString2(thumblen), getWidth() - 200, m_topmargin + 2, 200, 20, Justification::topRight);
}

void WaveformComponent::timerCallback()
{
	repaint();
}

void WaveformComponent::setFileCachedRange(std::pair<Range<double>, Range<double>> rng)
{
	m_file_cached = rng;
}

void WaveformComponent::setTimerEnabled(bool b)
{
	if (b == true)
		startTimer(100);
	else
		stopTimer();
}

void WaveformComponent::setViewRange(Range<double> rng)
{
	m_view_range = rng;
	m_waveimage = Image();
	repaint();
}


void WaveformComponent::mouseDown(const MouseEvent & e)
{
	m_mousedown = true;
	m_lock_timesel_set = true;
	double pos = viewXToNormalized(e.x);
	if (e.y < m_topmargin || e.mods.isCommandDown())
	{
		if (SeekCallback)
			SeekCallback(pos);
		m_didseek = true;
	}
	else
	{
		m_time_sel_drag_target = getTimeSelectionEdge(e.x, e.y);
		m_drag_time_start = pos;
		if (m_time_sel_drag_target == 0)
		{
			//m_time_sel_start = 0.0;
			//m_time_sel_end = 1.0;
		}
	}

	repaint();
}

void WaveformComponent::mouseUp(const MouseEvent & /*e*/)
{
	m_lock_timesel_set = false;
	m_mousedown = false;
	m_didseek = false;
	if (m_didchangetimeselection)
	{
		TimeSelectionChangedCallback(Range<double>(m_time_sel_start, m_time_sel_end), 1);
		m_didchangetimeselection = false;
	}
}

void WaveformComponent::mouseDrag(const MouseEvent & e)
{
	if (m_didseek == true)
		return;
	if (m_time_sel_drag_target == 0 && e.mods.isAnyModifierKeyDown()==false)
	{
		m_time_sel_start = m_drag_time_start;
		m_time_sel_end = viewXToNormalized(e.x);
	}
	if (m_time_sel_drag_target == 0 && e.mods.isShiftDown())
	{
		double diff = m_drag_time_start - viewXToNormalized(e.x);
		m_time_sel_start -= diff;
		m_time_sel_end -= diff;
		m_drag_time_start -= diff;
	}
	double curlen = m_time_sel_end-m_time_sel_start;
	
    if (m_time_sel_drag_target == 1)
	{
		m_time_sel_start = viewXToNormalized(e.x);
    }
	if (m_time_sel_drag_target == 2)
	{
		m_time_sel_end = viewXToNormalized(e.x);
    }
	if (m_time_sel_start > m_time_sel_end)
	{
		std::swap(m_time_sel_start, m_time_sel_end);
		if (m_time_sel_drag_target == 1)
			m_time_sel_drag_target = 2;
		else if (m_time_sel_drag_target == 2)
			m_time_sel_drag_target = 1;
	}
	m_time_sel_start = jlimit(0.0, 1.0, m_time_sel_start);
	m_time_sel_end = jlimit(0.0, 1.0, m_time_sel_end);

	if (TimeSelectionChangedCallback)
	{
		if (m_time_sel_end>m_time_sel_start)
			TimeSelectionChangedCallback(Range<double>(m_time_sel_start, m_time_sel_end), 0);
		else
			TimeSelectionChangedCallback(Range<double>(0.0, 1.0), 0);
	}
	m_didchangetimeselection = true;
	repaint();
}

void WaveformComponent::mouseMove(const MouseEvent & e)
{
	m_time_sel_drag_target = getTimeSelectionEdge(e.x, e.y);
	if (m_time_sel_drag_target == 0)
		setMouseCursor(MouseCursor::NormalCursor);
	if (m_time_sel_drag_target == 1)
		setMouseCursor(MouseCursor::LeftRightResizeCursor);
	if (m_time_sel_drag_target == 2)
		setMouseCursor(MouseCursor::LeftRightResizeCursor);

}

void WaveformComponent::mouseDoubleClick(const MouseEvent & e)
{
	m_time_sel_start = 0.0;
	m_time_sel_end = 1.0;
	TimeSelectionChangedCallback({ 0.0,1.0 }, 0);
	repaint();
}

Range<double> WaveformComponent::getTimeSelection()
{
	if (m_time_sel_start >= 0.0 && m_time_sel_end>m_time_sel_start + 0.001)
		return { m_time_sel_start, m_time_sel_end };
	return { 0.0, 1.0 };
}

void WaveformComponent::setTimeSelection(Range<double> rng)
{
	if (m_lock_timesel_set == true)
		return;
	if (rng.isEmpty())
		rng = { -1.0,1.0 };
	m_time_sel_start = rng.getStart();
	m_time_sel_end = rng.getEnd();
	repaint();
}

int WaveformComponent::getTimeSelectionEdge(int x, int y)
{
	int xcorleft = (int)jmap<double>(m_time_sel_start, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	int xcorright = (int)jmap<double>(m_time_sel_end, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	if (juce::Rectangle<int>(xcorleft - 5, m_topmargin, 10, getHeight() - m_topmargin).contains(x, y))
		return 1;
	if (juce::Rectangle<int>(xcorright - 5, m_topmargin, 10, getHeight() - m_topmargin).contains(x, y))
		return 2;
	return 0;
}

SpectralVisualizer::SpectralVisualizer()
{
	m_img = Image(Image::RGB, 500, 200, true);
}

void SpectralVisualizer::setState(const ProcessParameters & pars, int nfreqs, double samplerate)
{
	double t0 = Time::getMillisecondCounterHiRes();
	double hz = 440.0;
	int numharmonics = 40;
	double scaler = 1.0 / numharmonics;
	if (m_img.getWidth()!=getWidth() || m_img.getHeight()!=getHeight())
		m_img = Image(Image::RGB, getWidth(), getHeight(), true);
	if (m_nfreqs == 0 || nfreqs != m_nfreqs)
	{
		m_nfreqs = nfreqs;
		m_insamples = std::vector<REALTYPE>(nfreqs * 2);
		m_freqs1 = std::vector<REALTYPE>(nfreqs);
		m_freqs2 = std::vector<REALTYPE>(nfreqs);
		m_freqs3 = std::vector<REALTYPE>(nfreqs);
		m_fft = std::make_unique<FFT>(nfreqs*2);
		std::fill(m_insamples.begin(), m_insamples.end(), 0.0f);
		for (int i = 0; i < nfreqs; ++i)
		{
			for (int j = 0; j < numharmonics; ++j)
			{
				double oscgain = 1.0 - (1.0 / numharmonics)*j;
				m_insamples[i] += scaler * oscgain * sin(2 * 3.141592653 / samplerate * i* (hz + hz * j));
			}
		}
	}
	
	//std::fill(m_freqs1.begin(), m_freqs1.end(), 0.0f);
	//std::fill(m_freqs2.begin(), m_freqs2.end(), 0.0f);
	//std::fill(m_freqs3.begin(), m_freqs3.end(), 0.0f);
	//std::fill(m_fft->freq.begin(), m_fft->freq.end(), 0.0f);
	for (int i = 0; i < nfreqs; ++i)
	{
		m_fft->smp[i] = m_insamples[i];
	}
	m_fft->applywindow(W_HAMMING);
	m_fft->smp2freq();
	double ratio = pow(2.0f, pars.pitch_shift.cents / 1200.0f);
	spectrum_do_pitch_shift(pars, nfreqs, m_fft->freq.data(), m_freqs2.data(), ratio);
	spectrum_do_freq_shift(pars, nfreqs, samplerate, m_freqs2.data(), m_freqs1.data());
	spectrum_do_compressor(pars, nfreqs, m_freqs1.data(), m_freqs2.data());
	spectrum_spread(nfreqs, samplerate, m_freqs3, m_freqs2.data(), m_freqs1.data(), pars.spread.bandwidth);
	//if (pars.harmonics.enabled)
	//	spectrum_do_harmonics(pars, m_freqs3, nfreqs, samplerate, m_freqs1.data(), m_freqs2.data());
	//else spectrum_copy(nfreqs, m_freqs1.data(), m_freqs2.data());
	Graphics g(m_img);
	g.fillAll(Colours::black);
	g.setColour(Colours::white);
	for (int i = 0; i < nfreqs; ++i)
	{
		double binfreq = (samplerate / 2 / nfreqs)*i;
		double xcor = jmap<double>(binfreq, 0.0, samplerate / 2.0, 0.0, getWidth());
		double ycor = getHeight()- jmap<double>(m_freqs2[i], 0.0, nfreqs/128, 0.0, getHeight());
		ycor = jlimit<double>(0.0, getHeight(), ycor);
		g.drawLine(xcor, getHeight(), xcor, ycor, 1.0);
	}
	double t1 = Time::getMillisecondCounterHiRes();
	m_elapsed = t1 - t0;
	repaint();
}

void SpectralVisualizer::paint(Graphics & g)
{
	g.drawImage(m_img, 0, 0, getWidth(), getHeight(), 0, 0, m_img.getWidth(), m_img.getHeight());
	g.setColour(Colours::yellow);
	g.drawText(String(m_elapsed, 1)+" ms", 1, 1, getWidth(), 30, Justification::topLeft);
}

void SpectralChainEditor::paint(Graphics & g)
{
	g.fillAll(Colours::black);
	if (m_src == nullptr)
		return;
	
	int box_w = getWidth() / m_order.size();
	int box_h = getHeight();
	for (int i = 0; i < m_order.size(); ++i)
	{
		//if (i!=m_cur_index)
			drawBox(g, i, i*box_w, 0, box_w - 30, box_h);
		if (i<m_order.size() - 1)
			g.drawArrow(juce::Line<float>(i*box_w + (box_w - 30), box_h / 2, i*box_w + box_w, box_h / 2), 2.0f, 15.0f, 15.0f);
	}
	if (m_drag_x>=0 && m_drag_x<getWidth() && m_cur_index>=0)
		drawBox(g, m_cur_index, m_drag_x, 0, box_w - 30, box_h);
}

void SpectralChainEditor::setSource(StretchAudioSource * src)
{
	m_src = src;
	m_order = m_src->getSpectrumProcessOrder();
	repaint();
}

void SpectralChainEditor::mouseDown(const MouseEvent & ev)
{
	m_did_drag = false;
    int box_w = getWidth() / m_order.size();
	int box_h = getHeight();
	m_cur_index = ev.x / box_w;
	if (m_cur_index >= 0)
	{
		if (ModuleSelectedCallback)
			ModuleSelectedCallback(m_order[m_cur_index].m_index);
		juce::Rectangle<int> r(box_w*m_cur_index, 1, 12, 12);
		if (r.contains(ev.x, ev.y))
		{
			m_order[m_cur_index].m_enabled = !m_order[m_cur_index].m_enabled;
			m_src->setSpectrumProcessOrder(m_order);
			if (ModuleOrderOrEnabledChangedCallback)
				ModuleOrderOrEnabledChangedCallback();
			repaint();
            return;
		}
	}
	m_drag_x = -1;
	repaint();
}

void SpectralChainEditor::mouseDrag(const MouseEvent & ev)
{
    int box_w = getWidth() / m_order.size();
    juce::Rectangle<int> r(box_w*m_cur_index, 1, 12, 12);
    if (r.contains(ev.x, ev.y))
        return;
    if (m_cur_index >= 0 && m_cur_index < m_order.size())
	{
		
		int box_h = getHeight();
		int new_index = ev.x / box_w;
		if (new_index >= 0 && new_index < m_order.size() && new_index != m_cur_index)
		{
			std::swap(m_order[m_cur_index], m_order[new_index]);
			m_cur_index = new_index;
			m_did_drag = true;
			m_src->setSpectrumProcessOrder(m_order);
			if (ModuleOrderOrEnabledChangedCallback)
				ModuleOrderOrEnabledChangedCallback();
		}
		m_drag_x = ev.x;
		repaint();
	}
}

void SpectralChainEditor::mouseUp(const MouseEvent & ev)
{
	m_drag_x = -1;
	//m_cur_index = -1;
	repaint();
}

void SpectralChainEditor::drawBox(Graphics & g, int index, int x, int y, int w, int h)
{
	String txt;
	if (m_order[index].m_index == 0)
		txt = "Harmonics";
	if (m_order[index].m_index == 1)
		txt = "Tonal vs Noise";
	if (m_order[index].m_index == 2)
		txt = "Frequency shift";
	if (m_order[index].m_index == 3)
		txt = "Pitch shift";
	if (m_order[index].m_index == 4)
		txt = "Octaves";
	if (m_order[index].m_index == 5)
		txt = "Spread";
	if (m_order[index].m_index == 6)
		txt = "Filter";
	if (m_order[index].m_index == 7)
		txt = "Compressor";
	if (index == m_cur_index)
	{
		g.setColour(Colours::darkgrey);
		//g.fillRect(i*box_w, 0, box_w - 30, box_h - 1);
		g.fillRect(x, y, w, h);
	}
	g.setColour(Colours::white);
	g.drawRect(x, y, w, h);
	g.drawFittedText(txt, x,y,w,h, Justification::centred, 3);
	g.setColour(Colours::gold);
	g.drawRect(x + 2, y + 2, 12, 12);
	if (m_order[index].m_enabled == true)
	{
		g.drawLine(x+2, y+2, x+14, y+14);
		g.drawLine(x+2, y+14, x+14, y+2);
	}
	g.setColour(Colours::white);
}

ParameterComponent::ParameterComponent(AudioProcessorParameter * par, bool notifyOnlyOnRelease) : m_par(par)
{
	addAndMakeVisible(&m_label);
	m_labeldefcolor = m_label.findColour(Label::textColourId);
	m_label.setText(par->getName(50), dontSendNotification);
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(par);
	if (floatpar)
	{
		m_slider = std::make_unique<MySlider>(&floatpar->range);
		m_notify_only_on_release = notifyOnlyOnRelease;
		m_slider->setRange(floatpar->range.start, floatpar->range.end, floatpar->range.interval);
		m_slider->setValue(*floatpar, dontSendNotification);
		m_slider->addListener(this);
		m_slider->setDoubleClickReturnValue(true, floatpar->range.convertFrom0to1(par->getDefaultValue()));
		addAndMakeVisible(m_slider.get());
	}
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(par);
	if (intpar)
	{
		m_slider = std::make_unique<MySlider>();
		m_notify_only_on_release = notifyOnlyOnRelease;
		m_slider->setRange(intpar->getRange().getStart(), intpar->getRange().getEnd(), 1.0);
		m_slider->setValue(*intpar, dontSendNotification);
		m_slider->addListener(this);
		addAndMakeVisible(m_slider.get());
	}
	AudioParameterChoice* choicepar = dynamic_cast<AudioParameterChoice*>(par);
	if (choicepar)
	{

	}
	AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(par);
	if (boolpar)
	{
		m_togglebut = std::make_unique<ToggleButton>();
		m_togglebut->setToggleState(*boolpar, dontSendNotification);
		m_togglebut->addListener(this);
		m_togglebut->setButtonText(par->getName(50));
		addAndMakeVisible(m_togglebut.get());
	}
}

void ParameterComponent::resized()
{
	if (m_slider)
	{
		int labw = 200;
		if (getWidth() < 400)
			labw = 100;
		m_label.setBounds(0, 0, labw, 24);
		m_slider->setBounds(m_label.getRight() + 1, 0, getWidth() - 2 - m_label.getWidth(), 24);
	}
	if (m_togglebut)
		m_togglebut->setBounds(1, 0, getWidth() - 1, 24);
}

void ParameterComponent::sliderValueChanged(Slider * slid)
{
	if (m_notify_only_on_release == true)
		return;
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
	if (floatpar != nullptr)
		*floatpar = slid->getValue();
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
	if (intpar != nullptr)
		*intpar = slid->getValue();
}

void ParameterComponent::sliderDragStarted(Slider * slid)
{
	m_dragging = true;
}

void ParameterComponent::sliderDragEnded(Slider * slid)
{
	m_dragging = false;
	if (m_notify_only_on_release == false)
		return;
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
	if (floatpar != nullptr)
		*floatpar = slid->getValue();
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
	if (intpar != nullptr)
		*intpar = slid->getValue();
}

void ParameterComponent::buttonClicked(Button * but)
{
	AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(m_par);
	if (m_togglebut != nullptr && m_togglebut->getToggleState() != *boolpar)
	{
		*boolpar = m_togglebut->getToggleState();
	}
}

void ParameterComponent::updateComponent()
{
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
	if (floatpar != nullptr && m_slider != nullptr && m_dragging == false && (float)m_slider->getValue() != *floatpar)
	{
		m_slider->setValue(*floatpar, dontSendNotification);
	}
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
	if (intpar != nullptr && m_slider != nullptr && m_dragging == false && (int)m_slider->getValue() != *intpar)
	{
		m_slider->setValue(*intpar, dontSendNotification);
	}
	AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(m_par);
	if (m_togglebut != nullptr && m_togglebut->getToggleState() != *boolpar)
	{
		m_togglebut->setToggleState(*boolpar, dontSendNotification);
	}
}

void ParameterComponent::setHighLighted(bool b)
{
	if (b == false)
	{
		m_label.setColour(Label::textColourId, m_labeldefcolor);
		if (m_togglebut)
			m_togglebut->setColour(ToggleButton::textColourId, m_labeldefcolor);
	}
	else
	{
		m_label.setColour(Label::textColourId, Colours::yellow);
		if (m_togglebut)
			m_togglebut->setColour(ToggleButton::textColourId, Colours::yellow);
	}
}

MySlider::MySlider(NormalisableRange<float>* range) : m_range(range)
{
}

double MySlider::proportionOfLengthToValue(double x)
{
	if (m_range)
		return m_range->convertFrom0to1(x);
	return Slider::proportionOfLengthToValue(x);
}

double MySlider::valueToProportionOfLength(double x)
{
	if (m_range)
		return m_range->convertTo0to1(x);
	return Slider::valueToProportionOfLength(x);
}

PerfMeterComponent::PerfMeterComponent(PaulstretchpluginAudioProcessor * p) 
	: m_proc(p) 
{
    m_gradient.isRadial = false;
    m_gradient.addColour(0.0, Colours::red);
    m_gradient.addColour(0.25, Colours::yellow);
    m_gradient.addColour(1.0, Colours::green);
    
}

void PerfMeterComponent::paint(Graphics & g)
{
    m_gradient.point1 = {0.0f,0.0f};
    m_gradient.point2 = {(float)getWidth(),0.0f};
    g.fillAll(Colours::grey);
	double amt = m_proc->getPreBufferingPercent();
	g.setColour(Colours::green);
	int w = amt * getWidth();
    //g.setGradientFill(m_gradient);
    g.fillRect(0, 0, w, getHeight());
	g.setColour(Colours::white);
	g.drawRect(0, 0, getWidth(), getHeight());
	g.setFont(10.0f);
	if (m_proc->getPreBufferAmount()>0)
		g.drawText("PREBUFFER", 0, 0, getWidth(), getHeight(), Justification::centred);
	else
		g.drawText("NO PREBUFFER", 0, 0, getWidth(), getHeight(), Justification::centred);
}

void PerfMeterComponent::mouseDown(const MouseEvent & ev)
{
	PopupMenu bufferingmenu;
	int curbufamount = m_proc->getPreBufferAmount();
	bufferingmenu.addItem(100, "None", true, curbufamount == -1);
	bufferingmenu.addItem(101, "Small", true, curbufamount == 1);
	bufferingmenu.addItem(102, "Medium", true, curbufamount == 2);
	bufferingmenu.addItem(103, "Large", true, curbufamount == 3);
	bufferingmenu.addItem(104, "Very large", true, curbufamount == 4);
	bufferingmenu.addItem(105, "Huge", true, curbufamount == 5);
	int r = bufferingmenu.show();
	if (r >= 100 && r < 200)
	{
		if (r == 100)
			m_proc->m_use_backgroundbuffering = false;
		if (r > 100)
			m_proc->setPreBufferAmount(r - 100);
	}
}

void zoom_scrollbar::mouseDown(const MouseEvent &e)
{
	m_drag_start_x = e.x;
}

void zoom_scrollbar::mouseMove(const MouseEvent &e)
{
	auto ha = get_hot_area(e.x, e.y);
	if (ha == ha_left_edge || m_hot_area == ha_right_edge)
		setMouseCursor(MouseCursor::LeftRightResizeCursor);
	else
		setMouseCursor(MouseCursor::NormalCursor);
	if (ha != m_hot_area)
	{
		m_hot_area = ha;
		repaint();
	}
}

void zoom_scrollbar::mouseDrag(const MouseEvent &e)
{
	if (m_hot_area == ha_none)
		return;
	if (m_hot_area == ha_left_edge)
	{
		double new_left_edge = 1.0 / getWidth()*e.x;
		m_therange.setStart(jlimit(0.0, m_therange.getEnd() - 0.01, new_left_edge));
		repaint();
	}
	if (m_hot_area == ha_right_edge)
	{
		double new_right_edge = 1.0 / getWidth()*e.x;
		m_therange.setEnd(jlimit(m_therange.getStart() + 0.01, 1.0, new_right_edge));
		repaint();
	}
	if (m_hot_area == ha_handle)
	{
		double delta = 1.0 / getWidth()*(e.x - m_drag_start_x);
		//double old_start = m_start;
		//double old_end = m_end;
		double old_len = m_therange.getLength();
		m_therange.setStart(jlimit(0.0, 1.0 - old_len, m_therange.getStart() + delta));
		m_therange.setEnd(jlimit(old_len, m_therange.getStart() + old_len, m_therange.getEnd() + delta));
		m_drag_start_x = e.x;
		repaint();
	}
	if (RangeChanged)
		RangeChanged(m_therange);
}

void zoom_scrollbar::mouseEnter(const MouseEvent & event)
{
	m_hot_area = get_hot_area(event.x, event.y);
	repaint();
}

void zoom_scrollbar::mouseExit(const MouseEvent &)
{
	m_hot_area = ha_none;
	repaint();
}

void zoom_scrollbar::paint(Graphics &g)
{
	g.setColour(Colours::darkgrey);
	g.fillRect(0, 0, getWidth(), getHeight());
	int x0 = (int)(getWidth()*m_therange.getStart());
	int x1 = (int)(getWidth()*m_therange.getEnd());
	if (m_hot_area != ha_none)
		g.setColour(Colours::white);
	else g.setColour(Colours::lightgrey);
	g.fillRect(x0, 0, x1 - x0, getHeight());
}

void zoom_scrollbar::setRange(Range<double> rng, bool docallback)
{
	if (rng.isEmpty())
		return;
	m_therange = rng.constrainRange({ 0.0,1.0 });
	if (RangeChanged && docallback)
		RangeChanged(m_therange);
	repaint();
}

zoom_scrollbar::hot_area zoom_scrollbar::get_hot_area(int x, int)
{
	int x0 = (int)(getWidth()*m_therange.getStart());
	int x1 = (int)(getWidth()*m_therange.getEnd());
	if (is_in_range(x, x0 - 5, x0 + 5))
		return ha_left_edge;
	if (is_in_range(x, x1 - 5, x1 + 5))
		return ha_right_edge;
	if (is_in_range(x, x0 + 5, x1 - 5))
		return ha_handle;
	return ha_none;
}
