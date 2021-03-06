//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: conf.cpp,v 1.33.2.18 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QByteArray>

#include <sndfile.h>
#include <errno.h>
#include <stdio.h>

#include "app.h"
#include "transport.h"
#include "icons.h"
#include "globals.h"
#include "functions.h"
#include "drumedit.h"
#include "pianoroll.h"
#include "scoreedit.h"
#include "master/masteredit.h"
#include "listedit.h"
#include "cliplist/cliplist.h"
#include "arrangerview.h"
#include "marker/markerview.h"
#include "master/lmaster.h"
#include "bigtime.h"
#include "arranger.h"
#include "conf.h"
#include "gconfig.h"
#include "pitchedit.h"
#include "midiport.h"
#include "mididev.h"
#include "driver/audiodev.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "xml.h"
#include "waveedit.h"
#include "midi.h"
#include "midisyncimpl.h"
#include "midifilterimpl.h"
#include "midictrl.h"
#include "ctrlcombo.h"
#include "genset.h"
#include "midiitransform.h"
#include "synth.h"
#include "audio.h"
#include "sync.h"
#include "wave.h"
#include "midiseq.h"
#include "amixer.h"
#include "track.h"
#include "plugin.h"

namespace MusECore {

extern void writeMidiTransforms(int level, Xml& xml);
extern void readMidiTransform(Xml&);

extern void writeMidiInputTransforms(int level, Xml& xml);
extern void readMidiInputTransform(Xml&);

//---------------------------------------------------------
//   readGeometry
//---------------------------------------------------------

QRect readGeometry(Xml& xml, const QString& name)
      {
      QRect r(0, 0, 50, 50);
      int val;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        xml.parse1();
                        break;
                  case Xml::Attribut:
                        val = xml.s2().toInt();
                        if (tag == "x")
                              r.setX(val);
                        else if (tag == "y")
                              r.setY(val);
                        else if (tag == "w")
                              r.setWidth(val);
                        else if (tag == "h")
                              r.setHeight(val);
                        break;
                  case Xml::TagEnd:
                        if (tag == name)
                              return r;
                  default:
                        break;
                  }
            }
      return r;
      }


//---------------------------------------------------------
//   readColor
//---------------------------------------------------------

QColor readColor(Xml& xml)
       {
       int val, r=0, g=0, b=0;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token != Xml::Attribut)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Attribut:
                        val = xml.s2().toInt();
                        if (tag == "r")
                              r = val;
                        else if (tag == "g")
                              g = val;
                        else if (tag == "b")
                              b = val;
                        break;
                  default:
                        break;
                  }
            }

      return QColor(r, g, b);
      }

//---------------------------------------------------------
//   readController
//---------------------------------------------------------

static void readController(Xml& xml, int midiPort, int channel)
      {
      int id = 0;
      int val = CTRL_VAL_UNKNOWN;

      for (;;) {
            Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "val")
                              val = xml.parseInt();
                        else
                              xml.unknown("controller");
                        break;
                  case Xml::Attribut:
                        if (tag == "id")
                              id = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "controller") {
                              MidiPort* port = &MusEGlobal::midiPorts[midiPort];
                              val = port->limitValToInstrCtlRange(id, val);
                              // The value here will actually be sent to the device LATER, in MidiPort::setMidiDevice()
                              port->setHwCtrlState(channel, id, val);
                              return;
                              }
                  default:
                        return;
                  }
            }
      }

//---------------------------------------------------------
//   readPortChannel
//---------------------------------------------------------

static void readPortChannel(Xml& xml, int midiPort)
      {
      int idx = 0;  //torbenh
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "controller") {
                              readController(xml, midiPort, idx);
                              }
                        else
                              xml.unknown("MidiDevice");
                        break;
                  case Xml::Attribut:
                        if (tag == "idx")
                              idx = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "channel")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfigMidiDevice
//---------------------------------------------------------

static void readConfigMidiDevice(Xml& xml)
      {
      QString device;
      int rwFlags = 3;
      int openFlags = 1;
      int type = MidiDevice::ALSA_MIDI;

      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "name")
                              device = xml.parse1();
                        else if (tag == "type")
                              type = xml.parseInt();
                        else if (tag == "openFlags")
                              openFlags = xml.parseInt();
                        else if (tag == "rwFlags")             // Jack midi devs need this.
                              rwFlags = xml.parseInt();
                        else
                              xml.unknown("MidiDevice");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "mididevice") {
                              MidiDevice* dev = MusEGlobal::midiDevices.find(device, type);
                              
                              if(!dev)
                              {
                                if(type == MidiDevice::JACK_MIDI)
                                {
                                  if(MusEGlobal::debugMsg)
                                    fprintf(stderr, "readConfigMidiDevice: creating jack midi device %s with rwFlags:%d\n", device.toLatin1().constData(), rwFlags);
                                  dev = MidiJackDevice::createJackMidiDevice(device, rwFlags);  
                                }
                                else
                                if(type == MidiDevice::ALSA_MIDI)
                                {
                                  if(MusEGlobal::debugMsg)
                                    fprintf(stderr, "readConfigMidiDevice: creating ALSA midi device %s with rwFlags:%d\n", device.toLatin1().constData(), rwFlags);
                                  dev = MidiAlsaDevice::createAlsaMidiDevice(device, rwFlags);  
                                }
                              }                              
                              
                              if(MusEGlobal::debugMsg && !dev) 
                                fprintf(stderr, "readConfigMidiDevice: device not found %s\n", device.toLatin1().constData());
                              
                              if (dev) {
                                    dev->setOpenFlags(openFlags);
                                    }
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfigMidiPort
//---------------------------------------------------------

static void readConfigMidiPort(Xml& xml, bool onlyReadChannelState)
      {
      int idx = 0;
      QString device;
              
      // Let's be bold. New users have been confused by generic midi not enabling any patches and controllers.
      // I had said this may cause HW problems by sending out GM sysEx when really the HW might not be GM.
      // But this really needs to be done, one way or another. 
      // FIXME: TODO: Make this user-configurable!
      QString instrument("GM");
      
      int rwFlags = 3;
      int openFlags = 1;
      int dic = -1;   
      int doc = -1;
      
      MidiSyncInfo tmpSi;
      int type = MidiDevice::ALSA_MIDI;
      bool pre_mididevice_ver_found = false;
      
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        
                        // onlyReadChannelState added so it doesn't overwrite midi ports.   p4.0.41 Tim.
                        // Try to keep the controller information. But, this may need to be moved below.  
                        // Also may want to try to keep sync info, but that's a bit risky, so let's not for now.
                        if (tag == "channel") {
                              readPortChannel(xml, idx);
                              break;
                              }
                        else if (onlyReadChannelState){
                              xml.skip(tag);
                              break;
                              }
                              
                        if (tag == "name")
                              device = xml.parse1();
                        else if (tag == "type")
                        {
                              pre_mididevice_ver_found = true;
                              type = xml.parseInt();
                        }
                        else if (tag == "record") {         // old
                              pre_mididevice_ver_found = true;
                              bool f = xml.parseInt();
                              if (f)
                                    openFlags |= 2;
                              }
                        else if (tag == "openFlags")
                        {
                              pre_mididevice_ver_found = true;
                              openFlags = xml.parseInt();
                        }
                        else if (tag == "rwFlags")             // Jack midi devs need this.
                        {
                              pre_mididevice_ver_found = true;
                              rwFlags = xml.parseInt();
                        }
                        else if (tag == "defaultInChans")
                              dic = xml.parseInt(); 
                        else if (tag == "defaultOutChans")
                              doc = xml.parseInt(); 
                        else if (tag == "midiSyncInfo")
                              tmpSi.read(xml);
                        else if (tag == "instrument") {    // Obsolete
                              instrument = xml.parse1();
                              //MusEGlobal::midiPorts[idx].setInstrument(    // Moved below
                              //   registerMidiInstrument(instrument)
                              //   );
                              }
                        else if (tag == "midithru")
                        {
                              pre_mididevice_ver_found = true;
                              xml.parseInt(); // obsolete
                        }
                        //else if (tag == "channel") {
                        //      readPortChannel(xml, idx);   // Moved above
                        //      }
                        else
                              xml.unknown("MidiDevice");
                        break;
                  case Xml::Attribut:
                        if (tag == "idx") {
                              idx = xml.s2().toInt();
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "midiport") {
                              
                              if(onlyReadChannelState)      // p4.0.41
                                return;
                              
                              if (idx < 0 || idx >= MIDI_PORTS) {
                                    fprintf(stderr, "bad midi port %d (>%d)\n",
                                       idx, MIDI_PORTS);
                                    idx = 0;
                                    }
                              
                              MidiDevice* dev = MusEGlobal::midiDevices.find(device, pre_mididevice_ver_found ? type : -1);
                              
                              if(!dev && type == MidiDevice::JACK_MIDI)
                              {
                                if(MusEGlobal::debugMsg)
                                  fprintf(stderr, "readConfigMidiPort: creating jack midi device %s with rwFlags:%d\n", device.toLatin1().constData(), rwFlags);
                                dev = MidiJackDevice::createJackMidiDevice(device, rwFlags);  
                              }
                              
                              if(MusEGlobal::debugMsg && !dev)
                                fprintf(stderr, "readConfigMidiPort: device not found %s\n", device.toLatin1().constData());
                              
                              MidiPort* mp = &MusEGlobal::midiPorts[idx];

                              mp->setDefaultOutChannels(0); // reset output channel to take care of the case where no default is specified

                              mp->setInstrument(registerMidiInstrument(instrument));  
                              if(dic != -1)                      // p4.0.17 Leave them alone unless set by song.
                                mp->setDefaultInChannels(dic);
                              if(doc != -1)
                                // p4.0.17 Turn on if and when multiple output routes supported.
                                #if 0
                                mp->setDefaultOutChannels(doc);
                                #else
                                setPortExclusiveDefOutChan(idx, doc);
                                #endif
                                
                              mp->syncInfo().copyParams(tmpSi);
                              // p3.3.50 Indicate the port was found in the song file, even if no device is assigned to it.
                              mp->setFoundInSongFile(true);
                              
                              if (dev) {
                                    if(pre_mididevice_ver_found)
                                      dev->setOpenFlags(openFlags);
                                    MusEGlobal::midiSeq->msgSetMidiDevice(mp, dev);
                                    }
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   loadConfigMetronom
//---------------------------------------------------------

static void loadConfigMetronom(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "premeasures")
                              MusEGlobal::preMeasures = xml.parseInt();
                        else if (tag == "measurepitch")
                              MusEGlobal::measureClickNote = xml.parseInt();
                        else if (tag == "measurevelo")
                              MusEGlobal::measureClickVelo = xml.parseInt();
                        else if (tag == "beatpitch")
                              MusEGlobal::beatClickNote = xml.parseInt();
                        else if (tag == "beatvelo")
                              MusEGlobal::beatClickVelo = xml.parseInt();
                        else if (tag == "channel")
                              MusEGlobal::clickChan = xml.parseInt();
                        else if (tag == "port")
                              MusEGlobal::clickPort = xml.parseInt();
                        else if (tag == "precountEnable")
                              MusEGlobal::precountEnableFlag = xml.parseInt();
                        else if (tag == "fromMastertrack")
                              MusEGlobal::precountFromMastertrackFlag = xml.parseInt();
                        else if (tag == "signatureZ")
                              MusEGlobal::precountSigZ = xml.parseInt();
                        else if (tag == "signatureN")
                              MusEGlobal::precountSigN = xml.parseInt();
                        else if (tag == "prerecord")
                              MusEGlobal::precountPrerecord = xml.parseInt();
                        else if (tag == "preroll")
                              MusEGlobal::precountPreroll = xml.parseInt();
                        else if (tag == "midiClickEnable")
                              MusEGlobal::midiClickFlag = xml.parseInt();
                        else if (tag == "audioClickEnable")
                              MusEGlobal::audioClickFlag = xml.parseInt();
                        else if (tag == "audioClickVolume")
                              MusEGlobal::audioClickVolume = xml.parseFloat();
                        else if (tag == "measClickVolume")
                              MusEGlobal::measClickVolume = xml.parseFloat();
                        else if (tag == "beatClickVolume")
                              MusEGlobal::beatClickVolume = xml.parseFloat();
                        else if (tag == "accent1ClickVolume")
                              MusEGlobal::accent1ClickVolume = xml.parseFloat();
                        else if (tag == "accent2ClickVolume")
                              MusEGlobal::accent2ClickVolume = xml.parseFloat();
                        else if (tag == "clickSamples")
                              MusEGlobal::clickSamples = (MusEGlobal::ClickSamples)xml.parseInt();
                        else if (tag == "beatSample")
                              MusEGlobal::config.beatSample = xml.parse1();
                        else if (tag == "measSample")
                              MusEGlobal::config.measSample = xml.parse1();
                        else if (tag == "accent1Sample")
                              MusEGlobal::config.accent1Sample = xml.parse1();
                        else if (tag == "accent2Sample")
                              MusEGlobal::config.accent2Sample = xml.parse1();
                        else
                              xml.unknown("Metronome");
                        break;
                  case Xml::TagEnd:
                        if (tag == "metronom")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readSeqConfiguration
//---------------------------------------------------------

static void readSeqConfiguration(Xml& xml, bool skipMidiPorts)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "metronom")
                              loadConfigMetronom(xml);
                        else if (tag == "mididevice")
                              readConfigMidiDevice(xml);
                        else if (tag == "midiport")
                              readConfigMidiPort(xml, skipMidiPorts);
                        else if (tag == "rcStop")
                              MusEGlobal::rcStopNote = xml.parseInt();
                        else if (tag == "rcEnable")
                              MusEGlobal::rcEnable = xml.parseInt();
                        else if (tag == "rcRecord")
                              MusEGlobal::rcRecordNote = xml.parseInt();
                        else if (tag == "rcGotoLeft")
                              MusEGlobal::rcGotoLeftMarkNote = xml.parseInt();
                        else if (tag == "rcPlay")
                              MusEGlobal::rcPlayNote = xml.parseInt();
                        else if (tag == "rcSteprec")
                              MusEGlobal::rcSteprecNote = xml.parseInt();
                        else
                              xml.unknown("Seq");
                        break;
                  case Xml::TagEnd:
                        if (tag == "sequencer") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void readConfiguration(Xml& xml, bool doReadMidiPortConfig, bool doReadGlobalConfig)
      {
      if (doReadGlobalConfig) doReadMidiPortConfig=true;
      
      int mixers = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        /* the reading of configuration is split in two; read
                           "sequencer" and read ALL. The reason is that it is
                           possible to load a song without configuration. In
                           this case the <configuration> chapter in the song
                           file should be skipped. However the sub part
                           <sequencer> contains elements that are necessary
                           to preserve composition consistency. Mainly
                           midiport configuration and VOLUME.
                        */
                        if (tag == "sequencer") {
                              readSeqConfiguration(xml, !doReadMidiPortConfig);
                              break;
                              }
                        else if (tag == "waveTracksVisible")
                                 WaveTrack::setVisible((bool)xml.parseInt());
                        else if (tag == "auxTracksVisible")
                                 AudioAux::setVisible((bool)xml.parseInt());
                        else if (tag == "groupTracksVisible")
                                 AudioGroup::setVisible((bool)xml.parseInt());
                        else if (tag == "midiTracksVisible")
                                 MidiTrack::setVisible((bool)xml.parseInt());
                        else if (tag == "inputTracksVisible")
                                 AudioInput::setVisible((bool)xml.parseInt());
                        else if (tag == "outputTracksVisible")
                                 AudioOutput::setVisible((bool)xml.parseInt());
                        else if (tag == "synthTracksVisible")
                                 SynthI::setVisible((bool)xml.parseInt());
                        else if (tag == "bigtimeVisible")
                              MusEGlobal::config.bigTimeVisible = xml.parseInt();
                        else if (tag == "transportVisible")
                              MusEGlobal::config.transportVisible = xml.parseInt();
                        else if (tag == "mixer1Visible")
                              MusEGlobal::config.mixer1Visible = xml.parseInt();
                        else if (tag == "mixer2Visible")
                              MusEGlobal::config.mixer2Visible = xml.parseInt();
                        else if (tag == "markerVisible")
                              // I thought this was obsolete (done by song's toplevel list), but
                              // it's obviously needed. (flo)
                              MusEGlobal::config.markerVisible = xml.parseInt();
                        else if (tag == "arrangerVisible")
                              // same here.
                              MusEGlobal::config.arrangerVisible = xml.parseInt();
                        else if (tag == "geometryTransport")
                              MusEGlobal::config.geometryTransport = readGeometry(xml, tag);
                        else if (tag == "geometryBigTime")
                              MusEGlobal::config.geometryBigTime = readGeometry(xml, tag);
                        else if (tag == "Mixer") {
                              if(mixers == 0)
                                MusEGlobal::config.mixer1.read(xml);
                              else  
                                MusEGlobal::config.mixer2.read(xml);
                              ++mixers;
                              }
                        else if (tag == "geometryMain")
                              MusEGlobal::config.geometryMain = readGeometry(xml, tag);
                        else if (tag == "trackHeight")
                                 MusEGlobal::config.trackHeight = xml.parseInt();


                        else if (doReadMidiPortConfig==false) {
                              xml.skip(tag);
                              break;
                              }
                              
                              
                              
                              
                              
                        else if (tag == "midiInputDevice")
                              MusEGlobal::midiInputPorts = xml.parseInt();
                        else if (tag == "midiInputChannel")
                              MusEGlobal::midiInputChannel = xml.parseInt();
                        else if (tag == "midiRecordType")
                              MusEGlobal::midiRecordType = xml.parseInt();
                        else if (tag == "midiThruType")
                              MusEGlobal::midiThruType = xml.parseInt();
                        else if (tag == "midiFilterCtrl1")
                              MusEGlobal::midiFilterCtrl1 = xml.parseInt();
                        else if (tag == "midiFilterCtrl2")
                              MusEGlobal::midiFilterCtrl2 = xml.parseInt();
                        else if (tag == "midiFilterCtrl3")
                              MusEGlobal::midiFilterCtrl3 = xml.parseInt();
                        else if (tag == "midiFilterCtrl4")
                              MusEGlobal::midiFilterCtrl4 = xml.parseInt();
                        else if (tag == "mtctype")
                              MusEGlobal::mtcType= xml.parseInt();
                        else if (tag == "sendClockDelay")
                              MusEGlobal::syncSendFirstClockDelay = xml.parseUInt();
                        else if (tag == "extSync")
                              MusEGlobal::extSyncFlag.setValue(xml.parseInt());
                        else if (tag == "useJackTransport")
                              {
                              MusEGlobal::useJackTransport.setValue(xml.parseInt());
                              }
                        else if (tag == "jackTransportMaster")
                              {
                                MusEGlobal::jackTransportMaster = xml.parseInt();
                                if(MusEGlobal::audioDevice)
                                      MusEGlobal::audioDevice->setMaster(MusEGlobal::jackTransportMaster);      
                              }  
                        else if (tag == "syncRecFilterPreset")
                              {
                              int p = xml.parseInt();  
                              if(p >= 0 && p < MidiSyncInfo::TYPE_END)
                              {
                                MusEGlobal::syncRecFilterPreset = MidiSyncInfo::SyncRecFilterPresetType(p);
                                if(MusEGlobal::midiSeq)
                                  MusEGlobal::midiSeq->setSyncRecFilterPreset(MusEGlobal::syncRecFilterPreset);
                              }
                              }
                        else if (tag == "syncRecTempoValQuant")
                              {
                                double qv = xml.parseDouble();
                                MusEGlobal::syncRecTempoValQuant = qv;
                                if(MusEGlobal::midiSeq)
                                  MusEGlobal::midiSeq->setRecTempoValQuant(qv);
                              }
                        else if (tag == "mtcoffset") {
                              QString qs(xml.parse1());
                              QByteArray ba = qs.toLatin1();
                              const char* str = ba.constData();
                              int h, m, s, f, sf;
                              sscanf(str, "%d:%d:%d:%d:%d", &h, &m, &s, &f, &sf);
                              MusEGlobal::mtcOffset = MTC(h, m, s, f, sf);
                              }
                        else if (tag == "midiTransform")
                              readMidiTransform(xml);
                        else if (tag == "midiInputTransform")
                              readMidiInputTransform(xml);
                              
                        // don't insert else if(...) clauses between
                        // this line and "Global config stuff begins here".
                        else if (!doReadGlobalConfig) {
                              xml.skip(tag);
                              break;
                              }

                        // ---- Global and/or per-song config stuff ends here ----
                        
                        
                        
                        // ---- Global config stuff begins here ----

                        else if (tag == "pluginLadspaPathList")
                              MusEGlobal::config.pluginLadspaPathList = xml.parse1().split(":", QString::SkipEmptyParts);
                        else if (tag == "pluginDssiPathList")
                              MusEGlobal::config.pluginDssiPathList = xml.parse1().split(":", QString::SkipEmptyParts);
                        else if (tag == "pluginVstPathList")
                              MusEGlobal::config.pluginVstPathList = xml.parse1().split(":", QString::SkipEmptyParts);
                        else if (tag == "pluginLinuxVstPathList")
                              MusEGlobal::config.pluginLinuxVstPathList = xml.parse1().split(":", QString::SkipEmptyParts);
                        else if (tag == "pluginLv2PathList")
                              MusEGlobal::config.pluginLv2PathList = xml.parse1().split(":", QString::SkipEmptyParts);
                        
                        else if (tag == "preferredRouteNameOrAlias")
                              MusEGlobal::config.preferredRouteNameOrAlias = static_cast<MusEGlobal::RouteNameAliasPreference>(xml.parseInt());
                        else if (tag == "routerExpandVertically")
                              MusEGlobal::config.routerExpandVertically = xml.parseInt();
                        else if (tag == "routerGroupingChannels")
                        {
                              MusEGlobal::config.routerGroupingChannels = xml.parseInt();
                              // TODO: For now we only support maximum two channels grouping. Zero is an error.
                              if(MusEGlobal::config.routerGroupingChannels < 1)
                                MusEGlobal::config.routerGroupingChannels = 1;
                              if(MusEGlobal::config.routerGroupingChannels > 2)
                                MusEGlobal::config.routerGroupingChannels = 2;
                        }
                        else if (tag == "theme")
                              MusEGlobal::config.style = xml.parse1();
                        else if (tag == "autoSave")
                              MusEGlobal::config.autoSave = xml.parseInt();
                        else if (tag == "scrollableSubMenus")
                              MusEGlobal::config.scrollableSubMenus = xml.parseInt();
                        else if (tag == "liveWaveUpdate")
                              MusEGlobal::config.liveWaveUpdate = xml.parseInt();
                        else if (tag == "styleSheetFile")
                              MusEGlobal::config.styleSheetFile = xml.parse1();
                        else if (tag == "useOldStyleStopShortCut")
                              MusEGlobal::config.useOldStyleStopShortCut = xml.parseInt();
                        else if (tag == "moveArmedCheckBox")
                              MusEGlobal::config.moveArmedCheckBox = xml.parseInt();
                        else if (tag == "externalWavEditor")
                              MusEGlobal::config.externalWavEditor = xml.parse1();
                        else if (tag == "font0")
                              MusEGlobal::config.fonts[0].fromString(xml.parse1());
                        else if (tag == "font1")
                              MusEGlobal::config.fonts[1].fromString(xml.parse1());
                        else if (tag == "font2")
                              MusEGlobal::config.fonts[2].fromString(xml.parse1());
                        else if (tag == "font3")
                              MusEGlobal::config.fonts[3].fromString(xml.parse1());
                        else if (tag == "font4")
                              MusEGlobal::config.fonts[4].fromString(xml.parse1());
                        else if (tag == "font5")
                              MusEGlobal::config.fonts[5].fromString(xml.parse1());
                        else if (tag == "font6")
                              MusEGlobal::config.fonts[6].fromString(xml.parse1());
                        else if (tag == "globalAlphaBlend")
                              MusEGlobal::config.globalAlphaBlend = xml.parseInt();
                        else if (tag == "palette0")
                              MusEGlobal::config.palette[0] = readColor(xml);
                        else if (tag == "palette1")
                              MusEGlobal::config.palette[1] = readColor(xml);
                        else if (tag == "palette2")
                              MusEGlobal::config.palette[2] = readColor(xml);
                        else if (tag == "palette3")
                              MusEGlobal::config.palette[3] = readColor(xml);
                        else if (tag == "palette4")
                              MusEGlobal::config.palette[4] = readColor(xml);
                        else if (tag == "palette5")
                              MusEGlobal::config.palette[5] = readColor(xml);
                        else if (tag == "palette6")
                              MusEGlobal::config.palette[6] = readColor(xml);
                        else if (tag == "palette7")
                              MusEGlobal::config.palette[7] = readColor(xml);
                        else if (tag == "palette8")
                              MusEGlobal::config.palette[8] = readColor(xml);
                        else if (tag == "palette9")
                              MusEGlobal::config.palette[9] = readColor(xml);
                        else if (tag == "palette10")
                              MusEGlobal::config.palette[10] = readColor(xml);
                        else if (tag == "palette11")
                              MusEGlobal::config.palette[11] = readColor(xml);
                        else if (tag == "palette12")
                              MusEGlobal::config.palette[12] = readColor(xml);
                        else if (tag == "palette13")
                              MusEGlobal::config.palette[13] = readColor(xml);
                        else if (tag == "palette14")
                              MusEGlobal::config.palette[14] = readColor(xml);
                        else if (tag == "palette15")
                              MusEGlobal::config.palette[15] = readColor(xml);
                        else if (tag == "palette16")
                              MusEGlobal::config.palette[16] = readColor(xml);

                        else if (tag == "partColor0")
                              MusEGlobal::config.partColors[0] = readColor(xml);
                        else if (tag == "partColor1")
                              MusEGlobal::config.partColors[1] = readColor(xml);
                        else if (tag == "partColor2")
                              MusEGlobal::config.partColors[2] = readColor(xml);
                        else if (tag == "partColor3")
                              MusEGlobal::config.partColors[3] = readColor(xml);
                        else if (tag == "partColor4")
                              MusEGlobal::config.partColors[4] = readColor(xml);
                        else if (tag == "partColor5")
                              MusEGlobal::config.partColors[5] = readColor(xml);
                        else if (tag == "partColor6")
                              MusEGlobal::config.partColors[6] = readColor(xml);
                        else if (tag == "partColor7")
                              MusEGlobal::config.partColors[7] = readColor(xml);
                        else if (tag == "partColor8")
                              MusEGlobal::config.partColors[8] = readColor(xml);
                        else if (tag == "partColor9")
                              MusEGlobal::config.partColors[9] = readColor(xml);
                        else if (tag == "partColor10")
                              MusEGlobal::config.partColors[10] = readColor(xml);
                        else if (tag == "partColor11")
                              MusEGlobal::config.partColors[11] = readColor(xml);
                        else if (tag == "partColor12")
                              MusEGlobal::config.partColors[12] = readColor(xml);
                        else if (tag == "partColor13")
                              MusEGlobal::config.partColors[13] = readColor(xml);
                        else if (tag == "partColor14")
                              MusEGlobal::config.partColors[14] = readColor(xml);
                        else if (tag == "partColor15")
                              MusEGlobal::config.partColors[15] = readColor(xml);
                        else if (tag == "partColor16")
                              MusEGlobal::config.partColors[16] = readColor(xml);
                        else if (tag == "partColor17")
                              MusEGlobal::config.partColors[17] = readColor(xml);
                        
                        else if (tag == "partColorName0")
                              MusEGlobal::config.partColorNames[0] = xml.parse1();
                        else if (tag == "partColorName1")
                              MusEGlobal::config.partColorNames[1] = xml.parse1();
                        else if (tag == "partColorName2")
                              MusEGlobal::config.partColorNames[2] = xml.parse1();
                        else if (tag == "partColorName3")
                              MusEGlobal::config.partColorNames[3] = xml.parse1();
                        else if (tag == "partColorName4")
                              MusEGlobal::config.partColorNames[4] = xml.parse1();
                        else if (tag == "partColorName5")
                              MusEGlobal::config.partColorNames[5] = xml.parse1();
                        else if (tag == "partColorName6")
                              MusEGlobal::config.partColorNames[6] = xml.parse1();
                        else if (tag == "partColorName7")
                              MusEGlobal::config.partColorNames[7] = xml.parse1();
                        else if (tag == "partColorName8")
                              MusEGlobal::config.partColorNames[8] = xml.parse1();
                        else if (tag == "partColorName9")
                              MusEGlobal::config.partColorNames[9] = xml.parse1();
                        else if (tag == "partColorName10")
                              MusEGlobal::config.partColorNames[10] = xml.parse1();
                        else if (tag == "partColorName11")
                              MusEGlobal::config.partColorNames[11] = xml.parse1();
                        else if (tag == "partColorName12")
                              MusEGlobal::config.partColorNames[12] = xml.parse1();
                        else if (tag == "partColorName13")
                              MusEGlobal::config.partColorNames[13] = xml.parse1();
                        else if (tag == "partColorName14")
                              MusEGlobal::config.partColorNames[14] = xml.parse1();
                        else if (tag == "partColorName15")
                              MusEGlobal::config.partColorNames[15] = xml.parse1();
                        else if (tag == "partColorName16")
                              MusEGlobal::config.partColorNames[16] = xml.parse1();
                        else if (tag == "partColorName17")
                              MusEGlobal::config.partColorNames[17] = xml.parse1();
                        
                        else if (tag == "partCanvasBg")
                              MusEGlobal::config.partCanvasBg = readColor(xml);
                        else if (tag == "trackBg")
                              MusEGlobal::config.trackBg = readColor(xml);
                        else if (tag == "selectTrackBg")
                              MusEGlobal::config.selectTrackBg = readColor(xml);
                        else if (tag == "selectTrackFg")
                              MusEGlobal::config.selectTrackFg = readColor(xml);
                        else if (tag == "trackSectionDividerColor")
                              MusEGlobal::config.trackSectionDividerColor = readColor(xml);
                        else if (tag == "mixerBg")
                              MusEGlobal::config.mixerBg = readColor(xml);
                        else if (tag == "midiTrackLabelBg")
                              MusEGlobal::config.midiTrackLabelBg = readColor(xml);
                        else if (tag == "drumTrackLabelBg2")
                              MusEGlobal::config.drumTrackLabelBg = readColor(xml);
                        else if (tag == "newDrumTrackLabelBg2")
                              MusEGlobal::config.newDrumTrackLabelBg = readColor(xml);
                        else if (tag == "waveTrackLabelBg")
                              MusEGlobal::config.waveTrackLabelBg = readColor(xml);
                        else if (tag == "outputTrackLabelBg")
                              MusEGlobal::config.outputTrackLabelBg = readColor(xml);
                        else if (tag == "inputTrackLabelBg")
                              MusEGlobal::config.inputTrackLabelBg = readColor(xml);
                        else if (tag == "groupTrackLabelBg")
                              MusEGlobal::config.groupTrackLabelBg = readColor(xml);
                        else if (tag == "auxTrackLabelBg2")
                              MusEGlobal::config.auxTrackLabelBg = readColor(xml);
                        else if (tag == "synthTrackLabelBg")
                              MusEGlobal::config.synthTrackLabelBg = readColor(xml);
                        
                        else if (tag == "midiTrackBg")
                              MusEGlobal::config.midiTrackBg = readColor(xml);
                        else if (tag == "ctrlGraphFg")
                              MusEGlobal::config.ctrlGraphFg = readColor(xml);
                        else if (tag == "drumTrackBg")
                              MusEGlobal::config.drumTrackBg = readColor(xml);
                        else if (tag == "newDrumTrackBg")
                              MusEGlobal::config.newDrumTrackBg = readColor(xml);
                        else if (tag == "waveTrackBg")
                              MusEGlobal::config.waveTrackBg = readColor(xml);
                        else if (tag == "outputTrackBg")
                              MusEGlobal::config.outputTrackBg = readColor(xml);
                        else if (tag == "inputTrackBg")
                              MusEGlobal::config.inputTrackBg = readColor(xml);
                        else if (tag == "groupTrackBg")
                              MusEGlobal::config.groupTrackBg = readColor(xml);
                        else if (tag == "auxTrackBg")
                              MusEGlobal::config.auxTrackBg = readColor(xml);
                        else if (tag == "synthTrackBg")
                              MusEGlobal::config.synthTrackBg = readColor(xml);

                        else if (tag == "sliderDefaultColor")
                              MusEGlobal::config.sliderDefaultColor = readColor(xml);
                        else if (tag == "panSliderColor")
                              MusEGlobal::config.panSliderColor = readColor(xml);
                        else if (tag == "gainSliderColor")
                              MusEGlobal::config.gainSliderColor = readColor(xml);
                        else if (tag == "auxSliderColor")
                              MusEGlobal::config.auxSliderColor = readColor(xml);
                        else if (tag == "audioVolumeSliderColor")
                              MusEGlobal::config.audioVolumeSliderColor = readColor(xml);
                        else if (tag == "midiVolumeSliderColor")
                              MusEGlobal::config.midiVolumeSliderColor = readColor(xml);
                        else if (tag == "audioControllerSliderDefaultColor")
                              MusEGlobal::config.audioControllerSliderDefaultColor = readColor(xml);
                        else if (tag == "audioPropertySliderDefaultColor")
                              MusEGlobal::config.audioPropertySliderDefaultColor = readColor(xml);
                        else if (tag == "midiControllerSliderDefaultColor")
                              MusEGlobal::config.midiControllerSliderDefaultColor = readColor(xml);
                        else if (tag == "midiPropertySliderDefaultColor")
                              MusEGlobal::config.midiPropertySliderDefaultColor = readColor(xml);
                        else if (tag == "midiPatchSliderColor")
                              MusEGlobal::config.midiPatchSliderColor = readColor(xml);
                        else if (tag == "audioMeterPrimaryColor")
                              MusEGlobal::config.audioMeterPrimaryColor = readColor(xml);
                        else if (tag == "midiMeterPrimaryColor")
                              MusEGlobal::config.midiMeterPrimaryColor = readColor(xml);
                        
                        else if (tag == "extendedMidi")
                              MusEGlobal::config.extendedMidi = xml.parseInt();
                        else if (tag == "midiExportDivision")
                              MusEGlobal::config.midiDivision = xml.parseInt();
                        else if (tag == "copyright")
                              MusEGlobal::config.copyright = xml.parse1();
                        else if (tag == "smfFormat")
                              MusEGlobal::config.smfFormat = xml.parseInt();
                        else if (tag == "exp2ByteTimeSigs")
                              MusEGlobal::config.exp2ByteTimeSigs = xml.parseInt();
                        else if (tag == "expOptimNoteOffs")
                              MusEGlobal::config.expOptimNoteOffs = xml.parseInt();
                        else if (tag == "expRunningStatus")
                              MusEGlobal::config.expRunningStatus = xml.parseInt();
                        else if (tag == "importMidiSplitParts")
                              MusEGlobal::config.importMidiSplitParts = xml.parseInt();
                        else if (tag == "importDevNameMetas")
                              MusEGlobal::config.importDevNameMetas = xml.parseInt();
                        else if (tag == "importInstrNameMetas")
                              MusEGlobal::config.importInstrNameMetas = xml.parseInt();
                        else if (tag == "exportPortsDevices")
                              MusEGlobal::config.exportPortsDevices = xml.parseInt();
                        else if (tag == "exportPortDeviceSMF0")
                              MusEGlobal::config.exportPortDeviceSMF0 = xml.parseInt();
                        else if (tag == "exportModeInstr")
                              MusEGlobal::config.exportModeInstr = xml.parseInt();
                        else if (tag == "importMidiDefaultInstr")
                              MusEGlobal::config.importMidiDefaultInstr = xml.parse1();
                        
                        else if (tag == "showSplashScreen")
                              MusEGlobal::config.showSplashScreen = xml.parseInt();
                        else if (tag == "canvasShowPartType")
                              MusEGlobal::config.canvasShowPartType = xml.parseInt();
                        else if (tag == "canvasShowPartEvent")
                              MusEGlobal::config.canvasShowPartEvent = xml.parseInt();
                        else if (tag == "canvasShowGrid")
                              MusEGlobal::config.canvasShowGrid = xml.parseInt();
                        else if (tag == "canvasBgPixmap")
                              MusEGlobal::config.canvasBgPixmap = xml.parse1();
                        else if (tag == "canvasCustomBgList")
                              MusEGlobal::config.canvasCustomBgList = xml.parse1().split(";", QString::SkipEmptyParts);
                        else if (tag == "bigtimeForegroundcolor")
                              MusEGlobal::config.bigTimeForegroundColor = readColor(xml);
                        else if (tag == "bigtimeBackgroundcolor")
                              MusEGlobal::config.bigTimeBackgroundColor = readColor(xml);
                        else if (tag == "transportHandleColor")
                              MusEGlobal::config.transportHandleColor = readColor(xml);
                        else if (tag == "waveEditBackgroundColor")
                              MusEGlobal::config.waveEditBackgroundColor = readColor(xml);
                        else if (tag == "rulerBackgroundColor")
                              MusEGlobal::config.rulerBg = readColor(xml);
                        else if (tag == "rulerForegroundColor")
                              MusEGlobal::config.rulerFg = readColor(xml);
                        else if (tag == "rulerCurrentColor")
                              MusEGlobal::config.rulerCurrent = readColor(xml);

                        else if (tag == "waveNonselectedPart")
                              MusEGlobal::config.waveNonselectedPart = readColor(xml);
                        else if (tag == "wavePeakColor")
                              MusEGlobal::config.wavePeakColor = readColor(xml);
                        else if (tag == "waveRmsColor")
                              MusEGlobal::config.waveRmsColor = readColor(xml);
                        else if (tag == "wavePeakColorSelected")
                              MusEGlobal::config.wavePeakColorSelected = readColor(xml);
                        else if (tag == "waveRmsColorSelected")
                              MusEGlobal::config.waveRmsColorSelected = readColor(xml);

                        else if (tag == "partWaveColorPeak")
                              MusEGlobal::config.partWaveColorPeak = readColor(xml);
                        else if (tag == "partWaveColorRms")
                              MusEGlobal::config.partWaveColorRms = readColor(xml);
                        else if (tag == "partMidiDarkEventColor")
                              MusEGlobal::config.partMidiDarkEventColor = readColor(xml);
                        else if (tag == "partMidiLightEventColor")
                              MusEGlobal::config.partMidiLightEventColor = readColor(xml);

                        else if (tag == "midiCanvasBackgroundColor")
                              MusEGlobal::config.midiCanvasBg = readColor(xml);
                        else if (tag == "midiCanvasBeatColor")
                              MusEGlobal::config.midiCanvasBeatColor = readColor(xml);
                        else if (tag == "midiCanvasBarColor")
                              MusEGlobal::config.midiCanvasBarColor = readColor(xml);
                        else if (tag == "midiControllerViewBackgroundColor")
                              MusEGlobal::config.midiControllerViewBg = readColor(xml);
                        else if (tag == "drumListBackgroundColor")
                              MusEGlobal::config.drumListBg = readColor(xml);

                        else if (tag == "maxAliasedPointSize")
                              MusEGlobal::config.maxAliasedPointSize = xml.parseInt();
                        
                        //else if (tag == "midiSyncInfo")
                        //      readConfigMidiSyncInfo(xml);
                        /* Obsolete. done by song's toplevel list. arrangerview also handles arranger.
                        else if (tag == "arranger") {
                              if (MusEGlobal::muse && MusEGlobal::muse->arranger())
                                    MusEGlobal::muse->arranger()->readStatus(xml);
                              else
                                    xml.skip(tag);
                              }
                        */
                        else if (tag == "drumedit")
                              MusEGui::DrumEdit::readConfiguration(xml);
                        else if (tag == "pianoroll")
                              MusEGui::PianoRoll::readConfiguration(xml);
                        else if (tag == "scoreedit")
                              MusEGui::ScoreEdit::read_configuration(xml);
                        else if (tag == "masteredit")
                              MusEGui::MasterEdit::readConfiguration(xml);
                        else if (tag == "waveedit")
                              MusEGui::WaveEdit::readConfiguration(xml);
                        else if (tag == "listedit")
                              MusEGui::ListEdit::readConfiguration(xml);
                        else if (tag == "cliplistedit")
                              MusEGui::ClipListEdit::readConfiguration(xml);
                        else if (tag == "lmaster")
                              MusEGui::LMaster::readConfiguration(xml);
                        else if (tag == "marker")
                              MusEGui::MarkerView::readConfiguration(xml);
                        else if (tag == "arrangerview")
                              MusEGui::ArrangerView::readConfiguration(xml);
                        
                        else if (tag == "dialogs")
                              MusEGui::read_function_dialog_config(xml);
                        else if (tag == "shortcuts")
                              MusEGui::readShortCuts(xml);
                        else if (tag == "enableAlsaMidiDriver")
                              MusEGlobal::config.enableAlsaMidiDriver = xml.parseInt();
                        else if (tag == "division")
                              MusEGlobal::config.division = xml.parseInt();
                        else if (tag == "guiDivision")
                              MusEGlobal::config.guiDivision = xml.parseInt();
                        else if (tag == "rtcTicks")
                              MusEGlobal::config.rtcTicks = xml.parseInt();
                        else if (tag == "midiSendInit")
                              MusEGlobal::config.midiSendInit = xml.parseInt();
                        else if (tag == "warnInitPending")
                              MusEGlobal::config.warnInitPending = xml.parseInt();
                        else if (tag == "midiSendCtlDefaults")
                              MusEGlobal::config.midiSendCtlDefaults = xml.parseInt();
                        else if (tag == "midiSendNullParameters")
                              MusEGlobal::config.midiSendNullParameters = xml.parseInt();
                        else if (tag == "midiOptimizeControllers")
                              MusEGlobal::config.midiOptimizeControllers = xml.parseInt();
                        else if (tag == "warnIfBadTiming")
                              MusEGlobal::config.warnIfBadTiming = xml.parseInt();
                        else if (tag == "warnOnFileVersions")
                              MusEGlobal::config.warnOnFileVersions = xml.parseInt();
                        else if (tag == "lv2UiBehavior")
                              MusEGlobal::config.lv2UiBehavior = static_cast<MusEGlobal::CONF_LV2_UI_BEHAVIOR>(xml.parseInt());
                        else if (tag == "minMeter")
                              MusEGlobal::config.minMeter = xml.parseInt();
                        else if (tag == "minSlider")
                              MusEGlobal::config.minSlider = xml.parseDouble();
                        else if (tag == "freewheelMode")
                              MusEGlobal::config.freewheelMode = xml.parseInt();
                        else if (tag == "denormalProtection")
                              MusEGlobal::config.useDenormalBias = xml.parseInt();
                        else if (tag == "didYouKnow")
                              MusEGlobal::config.showDidYouKnow = xml.parseInt();
                        else if (tag == "outputLimiter")
                              MusEGlobal::config.useOutputLimiter = xml.parseInt();
                        else if (tag == "vstInPlace")
                              MusEGlobal::config.vstInPlace = xml.parseInt();
                        else if (tag == "dummyAudioSampleRate")
                              MusEGlobal::config.dummyAudioSampleRate = xml.parseInt();
                        else if (tag == "dummyAudioBufSize")
                              MusEGlobal::config.dummyAudioBufSize = xml.parseInt();
                        else if (tag == "minControlProcessPeriod")
                              MusEGlobal::config.minControlProcessPeriod = xml.parseUInt();
                        else if (tag == "guiRefresh")
                              MusEGlobal::config.guiRefresh = xml.parseInt();
                        else if (tag == "userInstrumentsDir")                        // Obsolete
                              MusEGlobal::config.userInstrumentsDir = xml.parse1();  // Keep for compatibility 
                        else if (tag == "startMode")
                              MusEGlobal::config.startMode = xml.parseInt();
                        else if (tag == "startSong")
                              MusEGlobal::config.startSong = xml.parse1();
                        else if (tag == "startSongLoadConfig")
                              MusEGlobal::config.startSongLoadConfig = xml.parseInt();                        
                        else if (tag == "newDrumRecordCondition")
                              MusEGlobal::config.newDrumRecordCondition = MusECore::newDrumRecordCondition_t(xml.parseInt());
                        else if (tag == "projectBaseFolder")
                              MusEGlobal::config.projectBaseFolder = xml.parse1();
                        else if (tag == "projectStoreInFolder")
                              MusEGlobal::config.projectStoreInFolder = xml.parseInt();
                        else if (tag == "useProjectSaveDialog")
                              MusEGlobal::config.useProjectSaveDialog = xml.parseInt();
                        else if (tag == "popupsDefaultStayOpen")
                              MusEGlobal::config.popupsDefaultStayOpen = xml.parseInt();
                        else if (tag == "leftMouseButtonCanDecrease")
                              MusEGlobal::config.leftMouseButtonCanDecrease = xml.parseInt();
                        else if (tag == "rangeMarkerWithoutMMB")
                              MusEGlobal::config.rangeMarkerWithoutMMB = xml.parseInt();
                        else if (tag == "addHiddenTracks")
                              MusEGlobal::config.addHiddenTracks = xml.parseInt();
                        else if (tag == "drumTrackPreference")
                              MusEGlobal::config.drumTrackPreference = (MusEGlobal::drumTrackPreference_t) xml.parseInt();
                        else if (tag == "unhideTracks")
                              MusEGlobal::config.unhideTracks = xml.parseInt();
                        else if (tag == "smartFocus")
                              MusEGlobal::config.smartFocus = xml.parseInt();
                        else if (tag == "borderlessMouse")
                              MusEGlobal::config.borderlessMouse = xml.parseInt();
                        else if (tag == "velocityPerNote")
                              MusEGlobal::config.velocityPerNote = xml.parseInt();
                        else if (tag == "plugin_groups")
                              MusEGlobal::readPluginGroupConfiguration(xml);
                        else if (tag == "mixdownPath")
                              MusEGlobal::config.mixdownPath = xml.parse1();

                        // ---- the following only skips obsolete entries ----
                        else if ((tag == "arranger") || (tag == "geometryPianoroll") || (tag == "geometryDrumedit"))
                              xml.skip(tag);
                        else if (tag == "mixerVisible")
                              xml.skip(tag);
                        else if (tag == "geometryMixer")
                              xml.skip(tag);
                        else if (tag == "txDeviceId")
                                xml.parseInt();
                        else if (tag == "rxDeviceId")
                                xml.parseInt();
                        else if (tag == "txSyncPort")
                                xml.parseInt();
                        else if (tag == "rxSyncPort")
                                xml.parseInt();
                        else if (tag == "syncgentype")
                              xml.parseInt();
                        else if (tag == "genMTCSync")
                              xml.parseInt();
                        else if (tag == "genMCSync")
                              xml.parseInt();
                        else if (tag == "genMMC")
                              xml.parseInt();
                        else if (tag == "acceptMTC")
                              xml.parseInt();
                        else if (tag == "acceptMMC")
                              xml.parseInt();
                        else if (tag == "acceptMC")
                              xml.parseInt();
                        else if ((tag == "samplerate") || (tag == "segmentsize") || (tag == "segmentcount"))
                              xml.parseInt();
                        else
                              xml.unknown("configuration");
                        break;
                  case Xml::Text:
                        printf("text <%s>\n", xml.s1().toLatin1().constData());
                        break;
                  case Xml::Attribut:
                        if (doReadMidiPortConfig==false)
                              break;
                        else if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "configuration") {
                              return;
                              }
                        break;
                  case Xml::Proc:
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------
bool readConfiguration()
{
    return readConfiguration(NULL);
}

bool readConfiguration(const char *configFile)
      {
      QByteArray ba;
      if (configFile == NULL)
      {
        ba = MusEGlobal::configName.toLatin1();
        configFile = ba.constData();
      }

      printf("Config File <%s>\n", configFile);
      FILE* f = fopen(configFile, "r");
      if (f == 0) {
            if (MusEGlobal::debugMsg || MusEGlobal::debugMode)
                  fprintf(stderr, "NO Config File <%s> found\n", configFile);

            if (MusEGlobal::config.userInstrumentsDir.isEmpty()) 
                  MusEGlobal::config.userInstrumentsDir = MusEGlobal::configPath + "/instruments";
            return true;
            }
      Xml xml(f);
      bool skipmode = true;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        fclose(f);
                        return true;
                  case Xml::TagStart:
                        if (skipmode && tag == "muse")
                              skipmode = false;
                        else if (skipmode)
                              break;
                        else if (tag == "configuration")
                              readConfiguration(xml,true, true /* read global config as well */);
                        else
                              xml.unknown("muse config");
                        break;
                  case Xml::Attribut:
                        if (tag == "version") {
                              int major = xml.s2().section('.', 0, 0).toInt();
                              int minor = xml.s2().section('.', 1, 1).toInt();
                              xml.setVersion(major, minor);
                              }
                        break;
                  case Xml::TagEnd:
                        if(xml.majorVersion() != xml.latestMajorVersion() || xml.minorVersion() != xml.latestMinorVersion())
                        {
                          fprintf(stderr, "\n***WARNING***\nLoaded config file version is %d.%d\nCurrent version is %d.%d\n"
                                  "Conversions may be applied!\n\n",
                                  xml.majorVersion(), xml.minorVersion(), 
                                  xml.latestMajorVersion(), xml.latestMinorVersion());
                        }
                        if (!skipmode && tag == "muse") {
                              fclose(f);
                              return false;
                              }
                  default:
                        break;
                  }
            }
      fclose(f);
      return true;
      }

//---------------------------------------------------------
//   writeSeqConfiguration
//---------------------------------------------------------

static void writeSeqConfiguration(int level, Xml& xml, bool writePortInfo)
      {
      xml.tag(level++, "sequencer");

      xml.tag(level++, "metronom");
      xml.intTag(level, "premeasures", MusEGlobal::preMeasures);
      xml.intTag(level, "measurepitch", MusEGlobal::measureClickNote);
      xml.intTag(level, "measurevelo", MusEGlobal::measureClickVelo);
      xml.intTag(level, "beatpitch", MusEGlobal::beatClickNote);
      xml.intTag(level, "beatvelo", MusEGlobal::beatClickVelo);
      xml.intTag(level, "channel", MusEGlobal::clickChan);
      xml.intTag(level, "port", MusEGlobal::clickPort);

      xml.intTag(level, "precountEnable", MusEGlobal::precountEnableFlag);
      xml.intTag(level, "fromMastertrack", MusEGlobal::precountFromMastertrackFlag);
      xml.intTag(level, "signatureZ", MusEGlobal::precountSigZ);
      xml.intTag(level, "signatureN", MusEGlobal::precountSigN);
      xml.intTag(level, "prerecord", MusEGlobal::precountPrerecord);
      xml.intTag(level, "preroll", MusEGlobal::precountPreroll);
      xml.intTag(level, "midiClickEnable", MusEGlobal::midiClickFlag);
      xml.intTag(level, "audioClickEnable", MusEGlobal::audioClickFlag);
      xml.floatTag(level, "audioClickVolume", MusEGlobal::audioClickVolume);
      xml.floatTag(level, "measClickVolume", MusEGlobal::measClickVolume);
      xml.floatTag(level, "beatClickVolume", MusEGlobal::beatClickVolume);
      xml.floatTag(level, "accent1ClickVolume", MusEGlobal::accent1ClickVolume);
      xml.floatTag(level, "accent2ClickVolume", MusEGlobal::accent2ClickVolume);
      xml.intTag(level, "clickSamples", MusEGlobal::clickSamples);
      xml.strTag(level, "beatSample", MusEGlobal::config.beatSample);
      xml.strTag(level, "measSample", MusEGlobal::config.measSample);
      xml.strTag(level, "accent1Sample", MusEGlobal::config.accent1Sample);
      xml.strTag(level, "accent2Sample", MusEGlobal::config.accent2Sample);
      xml.tag(level--, "/metronom");

      xml.intTag(level, "rcEnable",   MusEGlobal::rcEnable);
      xml.intTag(level, "rcStop",     MusEGlobal::rcStopNote);
      xml.intTag(level, "rcRecord",   MusEGlobal::rcRecordNote);
      xml.intTag(level, "rcGotoLeft", MusEGlobal::rcGotoLeftMarkNote);
      xml.intTag(level, "rcPlay",     MusEGlobal::rcPlayNote);
      xml.intTag(level, "rcSteprec",     MusEGlobal::rcSteprecNote);

      if (writePortInfo) {
            for(iMidiDevice imd = MusEGlobal::midiDevices.begin(); imd != MusEGlobal::midiDevices.end(); ++imd)
            {
              MidiDevice* dev = *imd;
              // TODO: For now, support only jack midi devices here. ALSA devices are different.
              //if(dev->deviceType() != MidiDevice::JACK_MIDI)
              if(dev->deviceType() != MidiDevice::JACK_MIDI && dev->deviceType() != MidiDevice::ALSA_MIDI)
                continue;

              xml.tag(level++, "mididevice");
              xml.strTag(level, "name",   dev->name());
              
              if(dev->deviceType() != MidiDevice::ALSA_MIDI)
                xml.intTag(level, "type", dev->deviceType());
              
              // Synths will not have been created yet when this is read! So, synthIs now store their own openFlags.
              if(dev->openFlags() != 1)
                xml.intTag(level, "openFlags", dev->openFlags());
              
              if(dev->deviceType() == MidiDevice::JACK_MIDI)
                xml.intTag(level, "rwFlags", dev->rwFlags());   // Need this. Jack midi devs are created by app.   p4.0.41 
              
              xml.etag(level--, "mididevice");
            }
        
            //
            // write information about all midi ports, their assigned
            // instruments and all managed midi controllers
            //
            for (int i = 0; i < MIDI_PORTS; ++i) {
                  bool used = false;
                  MidiPort* mport = &MusEGlobal::midiPorts[i];
                  MidiDevice* dev = mport->device();
                  // Route check by Tim. Port can now be used for routing even if no device.
                  // Also, check for other non-defaults and save port, to preserve settings even if no device.
                  if(!mport->noInRoute() || !mport->noOutRoute() || 
                  // p4.0.17 Since MidiPort:: and MidiDevice::writeRouting() ignore ports with no device, ignore them here, too.
                  // This prevents bogus routes from being saved and propagated in the med file.
                  // Hmm tough decision, should we save if no device? That would preserve routes in case user upgrades HW, 
                  //  or ALSA reorders or renames devices etc etc, then we have at least kept the track <-> port routes.
                     mport->defaultInChannels() != (1<<MIDI_CHANNELS)-1 ||   // p4.0.17 Default is now to connect to all channels.
                     mport->defaultOutChannels() ||
                     (!mport->instrument()->iname().isEmpty() && mport->instrument()->midiType() != MT_GM) ||
                     !mport->syncInfo().isDefault()) 
                    used = true;  
                  else  
                  {
                    MidiTrackList* tl = MusEGlobal::song->midis();
                    for (iMidiTrack it = tl->begin(); it != tl->end(); ++it) 
                    {
                      MidiTrack* t = *it;
                      if (t->outPort() == i) 
                      {
                        used = true;
                        break;
                      }
                    }
                  }  
                  
                  if (!used && !dev)
                        continue;
                  xml.tag(level++, "midiport idx=\"%d\"", i);
                  
                  if(mport->defaultInChannels() != (1<<MIDI_CHANNELS)-1)     // p4.0.17 Default is now to connect to all channels.
                    xml.intTag(level, "defaultInChans", mport->defaultInChannels());
                  if(mport->defaultOutChannels())
                    xml.intTag(level, "defaultOutChans", mport->defaultOutChannels());
                  
                  if(!mport->instrument()->iname().isEmpty() &&                      // Tim.
                     (mport->instrument()->iname() != "GM"))                         // FIXME: TODO: Make this user configurable.
                    xml.strTag(level, "instrument", mport->instrument()->iname());
                    
                  if (dev) {
                        xml.strTag(level, "name",   dev->name());
                        }
                  mport->syncInfo().write(level, xml);
                  // write out registered controller for all channels
                  MidiCtrlValListList* vll = mport->controller();
                  for (int k = 0; k < MIDI_CHANNELS; ++k) {
                        int min = k << 24;
                        int max = min + 0x100000;
                        bool found = false;
                        iMidiCtrlValList s = vll->lower_bound(min);
                        iMidiCtrlValList e = vll->lower_bound(max);
                        if (s != e) {
                              for (iMidiCtrlValList i = s; i != e; ++i) {
                                    int ctl = i->second->num();
                                    if(mport->drumController(ctl))  // Including internals like polyaftertouch
                                      ctl |= 0xff;
                                    // Don't bother saving these empty controllers since they are already always added!
                                    if(defaultManagedMidiController.find(ctl) != defaultManagedMidiController.end() 
                                        && i->second->hwVal() == CTRL_VAL_UNKNOWN)
                                      continue;
                                    if(!found)
                                    {
                                      xml.tag(level++, "channel idx=\"%d\"", k);
                                      found = true;
                                    }
                                    xml.tag(level++, "controller id=\"%d\"", i->second->num());
                                    if (i->second->hwVal() != CTRL_VAL_UNKNOWN)
                                          xml.intTag(level, "val", i->second->hwVal());
                                    xml.etag(level--, "controller");
                                    }
                              }
                        if(found)      
                          xml.etag(level--, "channel");
                        }
                  xml.etag(level--, "midiport");
                  }
            }
      xml.tag(level, "/sequencer");
      }
      
      
static void writeConfigurationColors(int level, MusECore::Xml& xml, bool partColorNames = true)
{
     for (int i = 0; i < 16; ++i) {
            char buffer[32];
            sprintf(buffer, "palette%d", i);
            xml.colorTag(level, buffer, MusEGlobal::config.palette[i]);
            }

      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            char buffer[32];
            sprintf(buffer, "partColor%d", i);
            xml.colorTag(level, buffer, MusEGlobal::config.partColors[i]);
            }

      if(partColorNames)
      {
        for (int i = 0; i < NUM_PARTCOLORS; ++i) {
              char buffer[32];
              sprintf(buffer, "partColorName%d", i);
              xml.strTag(level, buffer, MusEGlobal::config.partColorNames[i]);
              }
      }
      
      xml.colorTag(level, "partCanvasBg",  MusEGlobal::config.partCanvasBg);
      xml.colorTag(level, "trackBg",       MusEGlobal::config.trackBg);
      xml.colorTag(level, "selectTrackBg", MusEGlobal::config.selectTrackBg);
      xml.colorTag(level, "selectTrackFg", MusEGlobal::config.selectTrackFg);
      xml.colorTag(level, "trackSectionDividerColor", MusEGlobal::config.trackSectionDividerColor);

      xml.colorTag(level, "mixerBg",            MusEGlobal::config.mixerBg);
      xml.colorTag(level, "midiTrackLabelBg",   MusEGlobal::config.midiTrackLabelBg);
      xml.colorTag(level, "drumTrackLabelBg2",  MusEGlobal::config.drumTrackLabelBg);
      xml.colorTag(level, "newDrumTrackLabelBg2",MusEGlobal::config.newDrumTrackLabelBg);
      xml.colorTag(level, "waveTrackLabelBg",   MusEGlobal::config.waveTrackLabelBg);
      xml.colorTag(level, "outputTrackLabelBg", MusEGlobal::config.outputTrackLabelBg);
      xml.colorTag(level, "inputTrackLabelBg",  MusEGlobal::config.inputTrackLabelBg);
      xml.colorTag(level, "groupTrackLabelBg",  MusEGlobal::config.groupTrackLabelBg);
      xml.colorTag(level, "auxTrackLabelBg2",   MusEGlobal::config.auxTrackLabelBg);
      xml.colorTag(level, "synthTrackLabelBg",  MusEGlobal::config.synthTrackLabelBg);
      
      xml.colorTag(level, "midiTrackBg",   MusEGlobal::config.midiTrackBg);
      xml.colorTag(level, "ctrlGraphFg",   MusEGlobal::config.ctrlGraphFg);
      xml.colorTag(level, "drumTrackBg",   MusEGlobal::config.drumTrackBg);
      xml.colorTag(level, "newDrumTrackBg",MusEGlobal::config.newDrumTrackBg);
      xml.colorTag(level, "waveTrackBg",   MusEGlobal::config.waveTrackBg);
      xml.colorTag(level, "outputTrackBg", MusEGlobal::config.outputTrackBg);
      xml.colorTag(level, "inputTrackBg",  MusEGlobal::config.inputTrackBg);
      xml.colorTag(level, "groupTrackBg",  MusEGlobal::config.groupTrackBg);
      xml.colorTag(level, "auxTrackBg",    MusEGlobal::config.auxTrackBg);
      xml.colorTag(level, "synthTrackBg",  MusEGlobal::config.synthTrackBg);

      xml.colorTag(level, "sliderDefaultColor",  MusEGlobal::config.sliderDefaultColor);
      xml.colorTag(level, "panSliderColor",  MusEGlobal::config.panSliderColor);
      xml.colorTag(level, "gainSliderColor",  MusEGlobal::config.gainSliderColor);
      xml.colorTag(level, "auxSliderColor",  MusEGlobal::config.auxSliderColor);
      xml.colorTag(level, "audioVolumeSliderColor",  MusEGlobal::config.audioVolumeSliderColor);
      xml.colorTag(level, "midiVolumeSliderColor",  MusEGlobal::config.midiVolumeSliderColor);
      xml.colorTag(level, "audioControllerSliderDefaultColor",  MusEGlobal::config.audioControllerSliderDefaultColor);
      xml.colorTag(level, "audioPropertySliderDefaultColor",  MusEGlobal::config.audioPropertySliderDefaultColor);
      xml.colorTag(level, "midiControllerSliderDefaultColor",  MusEGlobal::config.midiControllerSliderDefaultColor);
      xml.colorTag(level, "midiPropertySliderDefaultColor",  MusEGlobal::config.midiPropertySliderDefaultColor);
      xml.colorTag(level, "midiPatchSliderColor",  MusEGlobal::config.midiPatchSliderColor);
      xml.colorTag(level, "audioMeterPrimaryColor",  MusEGlobal::config.audioMeterPrimaryColor);
      xml.colorTag(level, "midiMeterPrimaryColor",  MusEGlobal::config.midiMeterPrimaryColor);
      
      xml.colorTag(level, "transportHandleColor",  MusEGlobal::config.transportHandleColor);
      xml.colorTag(level, "bigtimeForegroundcolor", MusEGlobal::config.bigTimeForegroundColor);
      xml.colorTag(level, "bigtimeBackgroundcolor", MusEGlobal::config.bigTimeBackgroundColor);
      xml.colorTag(level, "waveEditBackgroundColor", MusEGlobal::config.waveEditBackgroundColor);
      xml.colorTag(level, "rulerBackgroundColor", MusEGlobal::config.rulerBg);
      xml.colorTag(level, "rulerForegroundColor", MusEGlobal::config.rulerFg);
      xml.colorTag(level, "rulerCurrentColor", MusEGlobal::config.rulerCurrent);

      xml.colorTag(level, "waveNonselectedPart", MusEGlobal::config.waveNonselectedPart);
      xml.colorTag(level, "wavePeakColor", MusEGlobal::config.wavePeakColor);
      xml.colorTag(level, "waveRmsColor", MusEGlobal::config.waveRmsColor);
      xml.colorTag(level, "wavePeakColorSelected", MusEGlobal::config.wavePeakColorSelected);
      xml.colorTag(level, "waveRmsColorSelected", MusEGlobal::config.waveRmsColorSelected);

      xml.colorTag(level, "partWaveColorPeak", MusEGlobal::config.partWaveColorPeak);
      xml.colorTag(level, "partWaveColorRms", MusEGlobal::config.partWaveColorRms);
      xml.colorTag(level, "partMidiDarkEventColor", MusEGlobal::config.partMidiDarkEventColor);
      xml.colorTag(level, "partMidiLightEventColor", MusEGlobal::config.partMidiLightEventColor);

      xml.colorTag(level, "midiCanvasBackgroundColor", MusEGlobal::config.midiCanvasBg);
      xml.colorTag(level, "midiCanvasBeatColor", MusEGlobal::config.midiCanvasBeatColor);
      xml.colorTag(level, "midiCanvasBarColor", MusEGlobal::config.midiCanvasBarColor);

      xml.colorTag(level, "midiControllerViewBackgroundColor", MusEGlobal::config.midiControllerViewBg);
      xml.colorTag(level, "drumListBackgroundColor", MusEGlobal::config.drumListBg);
}
      

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   writeGlobalConfiguration
//---------------------------------------------------------

void MusE::writeGlobalConfiguration() const
      {
      FILE* f = fopen(MusEGlobal::configName.toLatin1().constData(), "w");
      if (f == 0) {
            printf("save configuration to <%s> failed: %s\n",
               MusEGlobal::configName.toLatin1().constData(), strerror(errno));
            return;
            }
      MusECore::Xml xml(f);
      xml.header();
      xml.nput(0, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
      writeGlobalConfiguration(1, xml);
      xml.tag(1, "/muse");
      fclose(f);
      }

bool MusE::loadConfigurationColors(QWidget* parent)
{
  if(!parent)
    parent = this;
  QString file = QFileDialog::getOpenFileName(parent, tr("Load configuration colors"), QString(), tr("MusE color configuration files *.cfc (*.cfc)"));
  if(file.isEmpty())
    return false;
  
  if(QMessageBox::question(parent, QString("MusE"),
      tr("Color settings will immediately be replaced with any found in the file.\nAre you sure you want to proceed?"), tr("&Ok"), tr("&Cancel"),
      QString::null, 0, 1 ) == 1)
    return false;
  
  // Read, and return if error.
  if(MusECore::readConfiguration(file.toLatin1().constData()))   // True if error.
  {
    fprintf(stderr, "MusE::loadConfigurationColors failed\n");
    return false;
  }
  // Notify app, and write into configuration file.
  changeConfig(true); 
  return true;
}

bool MusE::saveConfigurationColors(QWidget* parent)
{
  if(!parent)
    parent = this;
  QString file = QFileDialog::getSaveFileName(parent, tr("Save configuration colors"), QString(), tr("MusE color configuration files *.cfc (*.cfc)"));
  if(file.isEmpty())
    return false;

  if(QFile::exists(file))
  {
    if(QMessageBox::question(parent, QString("MusE"),
        tr("File exists.\nDo you want to overwrite it?"), tr("&Ok"), tr("&Cancel"),
        QString::null, 0, 1 ) == 1)
      return false;
  }
  FILE* f = fopen(file.toLatin1().constData(), "w");
  if (f == 0) 
  {
    fprintf(stderr, "save configuration colors to <%s> failed: %s\n",
        file.toLatin1().constData(), strerror(errno));
    return false;
  }
  MusECore::Xml xml(f);
  xml.header();
  xml.nput(0, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
  xml.tag(1, "configuration");
  MusECore::writeConfigurationColors(2, xml, false); // Don't save part colour names.
  xml.etag(1, "configuration");
  xml.tag(0, "/muse");
  fclose(f);
  return true;
}
      
void MusE::writeGlobalConfiguration(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "configuration");

      xml.strTag(level, "pluginLadspaPathList", MusEGlobal::config.pluginLadspaPathList.join(":"));
      xml.strTag(level, "pluginDssiPathList", MusEGlobal::config.pluginDssiPathList.join(":"));
      xml.strTag(level, "pluginVstPathList", MusEGlobal::config.pluginVstPathList.join(":"));
      xml.strTag(level, "pluginLinuxVstPathList", MusEGlobal::config.pluginLinuxVstPathList.join(":"));
      xml.strTag(level, "pluginLv2PathList", MusEGlobal::config.pluginLv2PathList.join(":"));
                        
      xml.intTag(level, "enableAlsaMidiDriver", MusEGlobal::config.enableAlsaMidiDriver);
      xml.intTag(level, "division", MusEGlobal::config.division);
      xml.intTag(level, "rtcTicks", MusEGlobal::config.rtcTicks);
      xml.intTag(level, "midiSendInit", MusEGlobal::config.midiSendInit);
      xml.intTag(level, "warnInitPending", MusEGlobal::config.warnInitPending);
      xml.intTag(level, "midiSendCtlDefaults", MusEGlobal::config.midiSendCtlDefaults);
      xml.intTag(level, "midiSendNullParameters", MusEGlobal::config.midiSendNullParameters);
      xml.intTag(level, "midiOptimizeControllers", MusEGlobal::config.midiOptimizeControllers);
      xml.intTag(level, "warnIfBadTiming", MusEGlobal::config.warnIfBadTiming);
      xml.intTag(level, "warnOnFileVersions", MusEGlobal::config.warnOnFileVersions);
      xml.intTag(level, "minMeter", MusEGlobal::config.minMeter);
      xml.doubleTag(level, "minSlider", MusEGlobal::config.minSlider);
      xml.intTag(level, "freewheelMode", MusEGlobal::config.freewheelMode);
      xml.intTag(level, "denormalProtection", MusEGlobal::config.useDenormalBias);
      xml.intTag(level, "didYouKnow", MusEGlobal::config.showDidYouKnow);
      xml.intTag(level, "outputLimiter", MusEGlobal::config.useOutputLimiter);
      xml.intTag(level, "vstInPlace", MusEGlobal::config.vstInPlace);
      xml.intTag(level, "dummyAudioBufSize", MusEGlobal::config.dummyAudioBufSize);
      xml.intTag(level, "dummyAudioSampleRate", MusEGlobal::config.dummyAudioSampleRate);
      xml.uintTag(level, "minControlProcessPeriod", MusEGlobal::config.minControlProcessPeriod);
      xml.intTag(level, "guiRefresh", MusEGlobal::config.guiRefresh);
      
      xml.intTag(level, "extendedMidi", MusEGlobal::config.extendedMidi);
      xml.intTag(level, "midiExportDivision", MusEGlobal::config.midiDivision);
      xml.intTag(level, "guiDivision", MusEGlobal::config.guiDivision);
      xml.strTag(level, "copyright", MusEGlobal::config.copyright);
      xml.intTag(level, "smfFormat", MusEGlobal::config.smfFormat);
      xml.intTag(level, "expRunningStatus", MusEGlobal::config.expRunningStatus);
      xml.intTag(level, "exp2ByteTimeSigs", MusEGlobal::config.exp2ByteTimeSigs);
      xml.intTag(level, "expOptimNoteOffs", MusEGlobal::config.expOptimNoteOffs);
      xml.intTag(level, "importMidiSplitParts", MusEGlobal::config.importMidiSplitParts);
      xml.intTag(level, "importDevNameMetas", MusEGlobal::config.importDevNameMetas);
      xml.intTag(level, "importInstrNameMetas", MusEGlobal::config.importInstrNameMetas);
      xml.intTag(level, "exportPortsDevices", MusEGlobal::config.exportPortsDevices);
      xml.intTag(level, "exportPortDeviceSMF0", MusEGlobal::config.exportPortDeviceSMF0);
      xml.intTag(level, "exportModeInstr", MusEGlobal::config.exportModeInstr);
      xml.strTag(level, "importMidiDefaultInstr", MusEGlobal::config.importMidiDefaultInstr);
      xml.intTag(level, "startMode", MusEGlobal::config.startMode);
      xml.strTag(level, "startSong", MusEGlobal::config.startSong);
      xml.intTag(level, "startSongLoadConfig", MusEGlobal::config.startSongLoadConfig);
      xml.intTag(level, "newDrumRecordCondition", MusEGlobal::config.newDrumRecordCondition);
      xml.strTag(level, "projectBaseFolder", MusEGlobal::config.projectBaseFolder);
      xml.intTag(level, "projectStoreInFolder", MusEGlobal::config.projectStoreInFolder);
      xml.intTag(level, "useProjectSaveDialog", MusEGlobal::config.useProjectSaveDialog);
      xml.intTag(level, "midiInputDevice", MusEGlobal::midiInputPorts);
      xml.intTag(level, "midiInputChannel", MusEGlobal::midiInputChannel);
      xml.intTag(level, "midiRecordType", MusEGlobal::midiRecordType);
      xml.intTag(level, "midiThruType", MusEGlobal::midiThruType);
      xml.intTag(level, "midiFilterCtrl1", MusEGlobal::midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2", MusEGlobal::midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3", MusEGlobal::midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4", MusEGlobal::midiFilterCtrl4);
      
      xml.intTag(level, "preferredRouteNameOrAlias", static_cast<int>(MusEGlobal::config.preferredRouteNameOrAlias));
      xml.intTag(level, "routerExpandVertically", MusEGlobal::config.routerExpandVertically);
      xml.intTag(level, "routerGroupingChannels", MusEGlobal::config.routerGroupingChannels);
      
      xml.strTag(level, "theme", MusEGlobal::config.style);
      xml.intTag(level, "autoSave", MusEGlobal::config.autoSave);
      xml.strTag(level, "styleSheetFile", MusEGlobal::config.styleSheetFile);
      xml.strTag(level, "externalWavEditor", MusEGlobal::config.externalWavEditor);
      xml.intTag(level, "useOldStyleStopShortCut", MusEGlobal::config.useOldStyleStopShortCut);
      xml.intTag(level, "moveArmedCheckBox", MusEGlobal::config.moveArmedCheckBox);
      xml.intTag(level, "popupsDefaultStayOpen", MusEGlobal::config.popupsDefaultStayOpen);
      xml.intTag(level, "leftMouseButtonCanDecrease", MusEGlobal::config.leftMouseButtonCanDecrease);
      xml.intTag(level, "rangeMarkerWithoutMMB", MusEGlobal::config.rangeMarkerWithoutMMB);
      xml.intTag(level, "smartFocus", MusEGlobal::config.smartFocus);
      xml.intTag(level, "borderlessMouse", MusEGlobal::config.borderlessMouse);
      xml.intTag(level, "velocityPerNote", MusEGlobal::config.velocityPerNote);
      
      xml.intTag(level, "unhideTracks", MusEGlobal::config.unhideTracks);
      xml.intTag(level, "addHiddenTracks", MusEGlobal::config.addHiddenTracks);
      xml.intTag(level, "drumTrackPreference", MusEGlobal::config.drumTrackPreference);

      xml.intTag(level, "waveTracksVisible",  MusECore::WaveTrack::visible());
      xml.intTag(level, "auxTracksVisible",  MusECore::AudioAux::visible());
      xml.intTag(level, "groupTracksVisible",  MusECore::AudioGroup::visible());
      xml.intTag(level, "midiTracksVisible",  MusECore::MidiTrack::visible());
      xml.intTag(level, "inputTracksVisible",  MusECore::AudioInput::visible());
      xml.intTag(level, "outputTracksVisible",  MusECore::AudioOutput::visible());
      xml.intTag(level, "synthTracksVisible",  MusECore::SynthI::visible());
      xml.intTag(level, "trackHeight",  MusEGlobal::config.trackHeight);
      xml.intTag(level, "scrollableSubMenus", MusEGlobal::config.scrollableSubMenus);
      xml.intTag(level, "liveWaveUpdate", MusEGlobal::config.liveWaveUpdate);
      xml.intTag(level, "lv2UiBehavior", static_cast<int>(MusEGlobal::config.lv2UiBehavior));
      xml.strTag(level, "mixdownPath", MusEGlobal::config.mixdownPath);

      for (int i = 0; i < NUM_FONTS; ++i) {
            char buffer[32];
            sprintf(buffer, "font%d", i);
            xml.strTag(level, buffer, MusEGlobal::config.fonts[i].toString());
            }
            
      xml.intTag(level, "globalAlphaBlend", MusEGlobal::config.globalAlphaBlend);
      
      MusECore::writeConfigurationColors(level, xml);
      
      xml.intTag(level, "mtctype", MusEGlobal::mtcType);
      xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
        MusEGlobal::mtcOffset.h(), MusEGlobal::mtcOffset.m(), MusEGlobal::mtcOffset.s(),
        MusEGlobal::mtcOffset.f(), MusEGlobal::mtcOffset.sf());
      MusEGlobal::extSyncFlag.save(level, xml);
      
      xml.qrectTag(level, "geometryMain",      MusEGlobal::config.geometryMain);
      xml.qrectTag(level, "geometryTransport", MusEGlobal::config.geometryTransport);
      xml.qrectTag(level, "geometryBigTime",   MusEGlobal::config.geometryBigTime);

      xml.intTag(level, "bigtimeVisible", MusEGlobal::config.bigTimeVisible);
      xml.intTag(level, "transportVisible", MusEGlobal::config.transportVisible);
      
      xml.intTag(level, "mixer1Visible", MusEGlobal::config.mixer1Visible);
      xml.intTag(level, "mixer2Visible", MusEGlobal::config.mixer2Visible);
      MusEGlobal::config.mixer1.write(level, xml);
      MusEGlobal::config.mixer2.write(level, xml);

      xml.intTag(level, "showSplashScreen", MusEGlobal::config.showSplashScreen);
      xml.intTag(level, "canvasShowPartType", MusEGlobal::config.canvasShowPartType);
      xml.intTag(level, "canvasShowPartEvent", MusEGlobal::config.canvasShowPartEvent);
      xml.intTag(level, "canvasShowGrid", MusEGlobal::config.canvasShowGrid);
      xml.strTag(level, "canvasBgPixmap", MusEGlobal::config.canvasBgPixmap);
      xml.strTag(level, "canvasCustomBgList", MusEGlobal::config.canvasCustomBgList.join(";"));

      xml.intTag(level, "maxAliasedPointSize", MusEGlobal::config.maxAliasedPointSize);
      
      MusEGlobal::writePluginGroupConfiguration(level, xml);

      writeSeqConfiguration(level, xml, false);

      MusEGui::DrumEdit::writeConfiguration(level, xml);
      MusEGui::PianoRoll::writeConfiguration(level, xml);
      MusEGui::ScoreEdit::write_configuration(level, xml);
      MusEGui::MasterEdit::writeConfiguration(level, xml);
      MusEGui::WaveEdit::writeConfiguration(level, xml);
      MusEGui::ListEdit::writeConfiguration(level, xml);
      MusEGui::ClipListEdit::writeConfiguration(level, xml);
      MusEGui::LMaster::writeConfiguration(level, xml);
      MusEGui::MarkerView::writeConfiguration(level, xml);
      arrangerView->writeConfiguration(level, xml);
      
      MusEGui::write_function_dialog_config(level, xml);

      MusEGui::writeShortCuts(level, xml);
      xml.etag(level, "configuration");
      }

//---------------------------------------------------------
//   writeConfiguration
//    write song specific configuration
//---------------------------------------------------------

void MusE::writeConfiguration(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "configuration");

      xml.intTag(level, "midiInputDevice",  MusEGlobal::midiInputPorts);
      xml.intTag(level, "midiInputChannel", MusEGlobal::midiInputChannel);
      xml.intTag(level, "midiRecordType",   MusEGlobal::midiRecordType);
      xml.intTag(level, "midiThruType",     MusEGlobal::midiThruType);
      xml.intTag(level, "midiFilterCtrl1",  MusEGlobal::midiFilterCtrl1);
      xml.intTag(level, "midiFilterCtrl2",  MusEGlobal::midiFilterCtrl2);
      xml.intTag(level, "midiFilterCtrl3",  MusEGlobal::midiFilterCtrl3);
      xml.intTag(level, "midiFilterCtrl4",  MusEGlobal::midiFilterCtrl4);

      xml.intTag(level, "mtctype", MusEGlobal::mtcType);
      xml.nput(level, "<mtcoffset>%02d:%02d:%02d:%02d:%02d</mtcoffset>\n",
        MusEGlobal::mtcOffset.h(), MusEGlobal::mtcOffset.m(), MusEGlobal::mtcOffset.s(),
        MusEGlobal::mtcOffset.f(), MusEGlobal::mtcOffset.sf());
      xml.uintTag(level, "sendClockDelay", MusEGlobal::syncSendFirstClockDelay);
      xml.intTag(level, "useJackTransport", MusEGlobal::useJackTransport.value());
      xml.intTag(level, "jackTransportMaster", MusEGlobal::jackTransportMaster);
      xml.intTag(level, "syncRecFilterPreset", MusEGlobal::syncRecFilterPreset);
      xml.doubleTag(level, "syncRecTempoValQuant", MusEGlobal::syncRecTempoValQuant);
      MusEGlobal::extSyncFlag.save(level, xml);
      
      xml.intTag(level, "bigtimeVisible",   viewBigtimeAction->isChecked());
      xml.intTag(level, "transportVisible", viewTransportAction->isChecked());
      
      xml.geometryTag(level, "geometryMain", this); // FINDME: maybe remove this? do we want
                                                    // the main win to jump around when loading?
      if (transport)
            xml.geometryTag(level, "geometryTransport", transport);
      if (bigtime)
            xml.geometryTag(level, "geometryBigTime", bigtime);
      
      // i thought this was obsolete, but it seems to be necessary (flo)
      xml.intTag(level, "arrangerVisible", viewArrangerAction->isChecked());
      xml.intTag(level, "markerVisible", viewMarkerAction->isChecked());
      // but storing the geometry IS obsolete. this is really
      // done by TopLevel::writeConfiguration

      xml.intTag(level, "mixer1Visible",    viewMixerAAction->isChecked());
      xml.intTag(level, "mixer2Visible",    viewMixerBAction->isChecked());
      if (mixer1)
            mixer1->write(level, xml);
      if (mixer2)
            mixer2->write(level, xml);

      writeSeqConfiguration(level, xml, true);

      MusEGui::write_function_dialog_config(level, xml);

      writeMidiTransforms(level, xml);
      writeMidiInputTransforms(level, xml);
      xml.etag(level, "configuration");
      }

//---------------------------------------------------------
//   configMidiSync
//---------------------------------------------------------

void MusE::configMidiSync()
      {
      if (!midiSyncConfig)
        // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
        midiSyncConfig = new MusEGui::MidiSyncConfig;
        
      if (midiSyncConfig->isVisible()) {
          midiSyncConfig->raise();
          midiSyncConfig->activateWindow();
          }
      else
            midiSyncConfig->show();
      }

//---------------------------------------------------------
//   configMidiFile
//---------------------------------------------------------

void MusE::configMidiFile()
      {
      if (!midiFileConfig)
            // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
            midiFileConfig = new MusEGui::MidiFileConfig();
      midiFileConfig->updateValues();

      if (midiFileConfig->isVisible()) {
          midiFileConfig->raise();
          midiFileConfig->activateWindow();
          }
      else
          midiFileConfig->show();
      }

//---------------------------------------------------------
//   configGlobalSettings
//---------------------------------------------------------

void MusE::configGlobalSettings()
      {
      if (!globalSettingsConfig)
          // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
          globalSettingsConfig = new MusEGui::GlobalSettingsConfig();

      if (globalSettingsConfig->isVisible()) {
          globalSettingsConfig->raise();
          globalSettingsConfig->activateWindow();
          }
      else
          globalSettingsConfig->show();
      }


//---------------------------------------------------------
//   MidiFileConfig
//    config properties of exported midi files
//---------------------------------------------------------

MidiFileConfig::MidiFileConfig(QWidget* parent)
  : QDialog(parent), ConfigMidiFileBase()
      {
      setupUi(this);
      connect(buttonOk, SIGNAL(clicked()), SLOT(okClicked()));
      connect(buttonCancel, SIGNAL(clicked()), SLOT(cancelClicked()));
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void MidiFileConfig::updateValues()
      {
      importDefaultInstr->clear();
      for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
        if(!(*i)->isSynti())                            // Sorry, no synths for now.
          importDefaultInstr->addItem((*i)->iname());
      int idx = importDefaultInstr->findText(MusEGlobal::config.importMidiDefaultInstr);
      if(idx != -1)
        importDefaultInstr->setCurrentIndex(idx);
      
      QString defInstr = importDefaultInstr->currentText();
      if(!defInstr.isEmpty())
        MusEGlobal::config.importMidiDefaultInstr = defInstr;
      
      int divisionIdx = 2;
      switch(MusEGlobal::config.midiDivision) {
            case 96:  divisionIdx = 0; break;
            case 192:  divisionIdx = 1; break;
            case 384:  divisionIdx = 2; break;
            }
      divisionCombo->setCurrentIndex(divisionIdx);
      formatCombo->setCurrentIndex(MusEGlobal::config.smfFormat);
      extendedFormat->setChecked(MusEGlobal::config.extendedMidi);
      copyrightEdit->setText(MusEGlobal::config.copyright);
      runningStatus->setChecked(MusEGlobal::config.expRunningStatus);
      optNoteOffs->setChecked(MusEGlobal::config.expOptimNoteOffs);
      twoByteTimeSigs->setChecked(MusEGlobal::config.exp2ByteTimeSigs);
      splitPartsCheckBox->setChecked(MusEGlobal::config.importMidiSplitParts);
      newDrumsCheckbox->setChecked(MusEGlobal::config.importMidiNewStyleDrum);
      oldDrumsCheckbox->setChecked(!MusEGlobal::config.importMidiNewStyleDrum);
      importDevNameMetas->setChecked(MusEGlobal::config.importDevNameMetas);
      importInstrNameMetas->setChecked(MusEGlobal::config.importInstrNameMetas);
      exportPortDeviceSMF0->setChecked(MusEGlobal::config.exportPortDeviceSMF0);
      exportPortMetas->setChecked(MusEGlobal::config.exportPortsDevices & MusEGlobal::PORT_NUM_META);
      exportDeviceNameMetas->setChecked(MusEGlobal::config.exportPortsDevices & MusEGlobal::DEVICE_NAME_META);
      exportModeSysexes->setChecked(MusEGlobal::config.exportModeInstr & MusEGlobal::MODE_SYSEX);
      exportInstrumentNames->setChecked(MusEGlobal::config.exportModeInstr & MusEGlobal::INSTRUMENT_NAME_META);
          
      }

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void MidiFileConfig::okClicked()
      {
      QString defInstr = importDefaultInstr->currentText();
      if(!defInstr.isEmpty())
        MusEGlobal::config.importMidiDefaultInstr = defInstr;
      
      int divisionIdx = divisionCombo->currentIndex();

      int divisions[3] = { 96, 192, 384 };
      if (divisionIdx >= 0 && divisionIdx < 3)
            MusEGlobal::config.midiDivision = divisions[divisionIdx];
      MusEGlobal::config.extendedMidi = extendedFormat->isChecked();
      MusEGlobal::config.smfFormat    = formatCombo->currentIndex();
      MusEGlobal::config.copyright    = copyrightEdit->text();
      MusEGlobal::config.expRunningStatus = runningStatus->isChecked();
      MusEGlobal::config.expOptimNoteOffs = optNoteOffs->isChecked();
      MusEGlobal::config.exp2ByteTimeSigs = twoByteTimeSigs->isChecked();
      MusEGlobal::config.importMidiSplitParts = splitPartsCheckBox->isChecked();
      MusEGlobal::config.importMidiNewStyleDrum = newDrumsCheckbox->isChecked();
      
      MusEGlobal::config.importDevNameMetas = importDevNameMetas->isChecked();
      MusEGlobal::config.importInstrNameMetas = importInstrNameMetas->isChecked();
      MusEGlobal::config.exportPortDeviceSMF0 = exportPortDeviceSMF0->isChecked();  
      
      MusEGlobal::config.exportPortsDevices = 0;
      if(exportPortMetas->isChecked())
        MusEGlobal::config.exportPortsDevices |= MusEGlobal::PORT_NUM_META;
      if(exportDeviceNameMetas->isChecked())
        MusEGlobal::config.exportPortsDevices |= MusEGlobal::DEVICE_NAME_META;

      MusEGlobal::config.exportModeInstr = 0;
      if(exportModeSysexes->isChecked())
        MusEGlobal::config.exportModeInstr |= MusEGlobal::MODE_SYSEX;
      if(exportInstrumentNames->isChecked())
        MusEGlobal::config.exportModeInstr |= MusEGlobal::INSTRUMENT_NAME_META;
      
      MusEGlobal::muse->changeConfig(true);  // write config file
      close();
      }

//---------------------------------------------------------
//   cancelClicked
//---------------------------------------------------------

void MidiFileConfig::cancelClicked()
      {
      close();
      }

} // namespace MusEGui


namespace MusEGlobal {

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MixerConfig::write(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "Mixer");

      xml.strTag(level, "name", name);
      
      xml.qrectTag(level, "geometry", geometry);
      
      xml.intTag(level, "showMidiTracks",   showMidiTracks);
      xml.intTag(level, "showDrumTracks",   showDrumTracks);
      xml.intTag(level, "showNewDrumTracks",   showNewDrumTracks);
      xml.intTag(level, "showInputTracks",  showInputTracks);
      xml.intTag(level, "showOutputTracks", showOutputTracks);
      xml.intTag(level, "showWaveTracks",   showWaveTracks);
      xml.intTag(level, "showGroupTracks",  showGroupTracks);
      xml.intTag(level, "showAuxTracks",    showAuxTracks);
      xml.intTag(level, "showSyntiTracks",  showSyntiTracks);

      xml.intTag(level, "displayOrder", displayOrder);

      xml.etag(level, "Mixer");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MixerConfig::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "name")
                              name = xml.parse1();
                        else if (tag == "geometry")
                              geometry = readGeometry(xml, tag);
                        else if (tag == "showMidiTracks")
                              showMidiTracks = xml.parseInt();
                        else if (tag == "showDrumTracks")
                              showDrumTracks = xml.parseInt();
                        else if (tag == "showNewDrumTracks")
                              showNewDrumTracks = xml.parseInt();
                        else if (tag == "showInputTracks")
                              showInputTracks = xml.parseInt();
                        else if (tag == "showOutputTracks")
                              showOutputTracks = xml.parseInt();
                        else if (tag == "showWaveTracks")
                              showWaveTracks = xml.parseInt();
                        else if (tag == "showGroupTracks")
                              showGroupTracks = xml.parseInt();
                        else if (tag == "showAuxTracks")
                              showAuxTracks = xml.parseInt();
                        else if (tag == "showSyntiTracks")
                              showSyntiTracks = xml.parseInt();
                        else if (tag == "displayOrder")
                              displayOrder = (DisplayOrder)xml.parseInt();
                        else if (tag == "StripName")
                              stripOrder.append(xml.parse1());
                        else if (tag == "StripVisible")
                              stripVisibility.append(xml.parseInt() == 0 ? false : true );
                        else
                              xml.unknown("Mixer");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "Mixer")
                            return;
                  default:
                        break;
                  }
            }
      
      }

} // namespace MusEGlobal

