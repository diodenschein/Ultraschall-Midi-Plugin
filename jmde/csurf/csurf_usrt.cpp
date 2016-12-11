/*
** reaper_csurf
** BCFF2k support
** Copyright (C) 2006-2008 Cockos Incorporated
** License: LGPL.
*/


#include "csurf.h"
#include "Helpers.h"

static bool g_csurf_mcpmode=false; // we may wish to allow an action to set this





class CSurf_USRT : public IReaperControlSurface
{
    int m_midi_in_dev,m_midi_out_dev;
    int m_offset, m_size;
    midi_Output *m_midiout;
    midi_Input *m_midiin;

    WDL_String descspace;
    char configtmp[1024];

    void OnMIDIEvent(MIDI_event_t *evt)
    {
      if ((evt->midi_message[0]&0xf0) == 0xB0)
      {
     //   int bank=evt->midi_message[0]&0xf;
          int bank = 0;

//        if (evt->midi_message[1] >= 0x51 && evt->midi_message[1] <= 0x58) // volume set
//        {
//          int trackid=(evt->midi_message[1]-0x51)+bank*8 + m_offset;
//          m_vol_lastpos[(trackid)&0xff]=evt->midi_message[2];
//          m_vol_lasttouch[(trackid)&0xff]=GetTickCount();
//          MediaTrack *tr=CSurf_TrackFromID(trackid,g_csurf_mcpmode);
//          if (tr) CSurf_SetSurfaceVolume(tr,CSurf_OnVolumeChange(tr,charToVol(evt->midi_message[2]),false),this);
//        }
//        else if (evt->midi_message[1] >= 0x21 && evt->midi_message[1] <= 0x28) // pan reset
//        {
//          int trackid=((evt->midi_message[1]-0x21)&7)+bank*8 + m_offset;
//          m_pan_lasttouch[trackid&0xff]=GetTickCount();
//          MediaTrack *tr=CSurf_TrackFromID(trackid,g_csurf_mcpmode);
//          if (tr) CSurf_SetSurfacePan(tr,CSurf_OnPanChange(tr,0.0,false),NULL);
//        }
//      else
        if (evt->midi_message[1] >= 0x01 && evt->midi_message[1] <= 0x08) // mute/solo
        {
          int trackid=((evt->midi_message[1]-0x00)&0x07) + m_offset;
//          int wi=(evt->midi_message[1]-0x00)&0x08;
          MediaTrack *tr=CSurf_TrackFromID(trackid,g_csurf_mcpmode);

          if (tr)
          {
//            if (!wi)
//              CSurf_SetSurfaceSolo(tr,CSurf_OnSoloChange(tr,evt->midi_message[2]>=0x00),this);
//            else
              CSurf_SetSurfaceMute(tr,CSurf_OnMuteChange(tr,evt->midi_message[2]>=0x00),this);
          }
        }
//        else if (evt->midi_message[1] >= 0x01 && evt->midi_message[1] <= 0x08) // pan set
//        {
//          int trackid=(evt->midi_message[1]-0x01)+bank*8 + m_offset;
//          m_pan_lasttouch[trackid&0xff]=GetTickCount();
//          m_pan_lastpos[trackid&0xff]=evt->midi_message[2];
//
//          MediaTrack *tr=CSurf_TrackFromID(trackid,g_csurf_mcpmode);
//          if (tr) CSurf_SetSurfacePan(tr,CSurf_OnPanChange(tr,charToPan(evt->midi_message[2]),false),this);
//        }
//        else if (evt->midi_message[1] == 0x59) CSurf_OnPlay();
//        else if (evt->midi_message[1] == 0x5a) CSurf_OnStop();
//        else if (evt->midi_message[1] == 0x5b) 
//        {
//          CSurf_OnRew(1);
//          evt->midi_message[2]=0;
//          if (m_midiout) m_midiout->SendMsg(evt,-1);
//        }
//        else if (evt->midi_message[1] == 0x5c) 
//        {
//          CSurf_OnFwd(1);
//          evt->midi_message[2]=0;
//          if (m_midiout) m_midiout->SendMsg(evt,-1);
//        }
      }
    }

public:
  CSurf_USRT(int offset, int size, int indev, int outdev, int *errStats)
  {
    m_offset=offset;
    m_size=size;
    m_midi_in_dev=indev;
    m_midi_out_dev=outdev;


    //create midi hardware access
    m_midiin = m_midi_in_dev >= 0 ? CreateMIDIInput(m_midi_in_dev) : NULL;
    m_midiout = m_midi_out_dev >= 0 ? CreateThreadedMIDIOutput(CreateMIDIOutput(m_midi_out_dev,false,NULL)) : NULL;

    if (errStats)
    {
      if (m_midi_in_dev >=0  && !m_midiin) *errStats|=1;
      if (m_midi_out_dev >=0  && !m_midiout) *errStats|=2;
    }

    if (m_midiin)
      m_midiin->start();

  }
  ~CSurf_USRT()
  {
    if (m_midiout)
    {
        m_midiout->Send(0xb0,0x7F,0x7F,-1); //Tell Arduino we war closing
        Sleep(5);
    }
    delete m_midiout;
    delete m_midiin;
  }


  const char *GetTypeString() { return "USRT"; }
  const char *GetDescString()
  {
    descspace.Set("Ultraschall Midi Controller");
    char tmp[512];
    sprintf(tmp," (dev %d,%d)",m_midi_in_dev,m_midi_out_dev);
    descspace.Append(tmp);
    return descspace.Get();     
  }
  const char *GetConfigString() // string of configuration data
  {
    sprintf(configtmp,"%d %d %d %d",m_offset,m_size,m_midi_in_dev,m_midi_out_dev);      
    return configtmp;
  }

  void CloseNoReset() 
  {
    delete m_midiout;
    delete m_midiin;
    m_midiout=0;
    m_midiin=0;
  }

  void Run()
  {
    if (m_midiin)
    {
      m_midiin->SwapBufs(timeGetTime());
      int l=0;
      MIDI_eventlist *list=m_midiin->GetReadBuf();
      MIDI_event_t *evts;
      while ((evts=list->EnumItems(&l))) OnMIDIEvent(evts);
    }
  }

  void SetTrackListChange() { } // not used

#define FIXID(id) int id=CSurf_TrackToID(trackid,g_csurf_mcpmode); int oid=id; id -= m_offset;

  void SetSurfaceVolume(MediaTrack *trackid, double volume) 
  {
//    FIXID(id)
//    if (m_midiout && id >= 0 && id < 256 && id < m_size)
//    {
//      unsigned char volch=volToChar(volume);
//
//      if (m_vol_lastpos[id]!=volch)
//      {
//        m_vol_lastpos[id]=volch;
//        m_midiout->Send(0xB0+id/8,0x51+(id&7),volch,-1);
//      }
//    }
  }
  void SetSurfaceMute(MediaTrack *trackid, bool mute) 
  { 
    FIXID(id)
    if (m_midiout && id>=0 && id < 256 && id < m_size)
    {
      m_midiout->Send(0xb0,(id&7),mute?0x7f:0,-1);
    }
  }
  void SetSurfaceSelected(MediaTrack *trackid, bool selected) 
  {
    // not used
  }
  void SetSurfaceSolo(MediaTrack *trackid, bool solo) 
  { 
//    FIXID(id)
//    if (m_midiout && id>=0 && id < 256 && id < m_size)
//    {
//      m_midiout->Send(0xb0+id/8,0x41+(id&7),solo?0x7f:0,-1);
//    }
  }
  void SetSurfaceRecArm(MediaTrack *trackid, bool recarm) 
  { 
    FIXID(id)
    if (m_midiout && id>=0 && id < 256 && id < m_size)
    {
        m_midiout->Send(0xb0,((id&7)+8),recarm?0x7f:0,-1);
    }
  }
  void SetPlayState(bool play, bool pause, bool rec) 
  { 
    if (m_midiout)
    {
      m_midiout->Send(0xb0,0x59,play?0x7f:0,-1);
      m_midiout->Send(0xb0,0x5a,pause?0x7f:0,-1);
      m_midiout->Send(0xb0,0x5b,rec?0x7f:0,-1);
    }
  }
  void SetRepeatState(bool rep) 
  { 
    // not used
  }

  void SetTrackTitle(MediaTrack *trackid, const char *title) { }
  bool GetTouchState(MediaTrack *trackid, int isPan) 
  { 
//    FIXID(id)
//    unsigned int *wb=isPan == 1 ? m_pan_lasttouch : m_vol_lasttouch;
//    if (oid >= 0 && oid < 256)
//    {
//      DWORD now=timeGetTime();
//      if ((now<wb[oid]+3000 && now >= wb[oid]-1000)) // fake touch, go for 3s after last movement
//        return true;
//    }
    return false;
  }

  void SetAutoMode(int mode) { }

  void ResetCachedVolPanStates() 
  { 
  }
  void OnTrackSelection(MediaTrack *trackid) 
  { 
  }
  
  bool IsKeyDown(int key) 
  { 
    return false; 
  }


};


static void parseParms(const char *str, int parms[4])
{
  parms[0]=0;
  parms[1]=9;
  parms[2]=parms[3]=-1;

  const char *p=str;
  if (p)
  {
    int x=0;
    while (x<4)
    {
      while (*p == ' ') p++;
      if ((*p < '0' || *p > '9') && *p != '-') break;
      parms[x++]=atoi(p);
      while (*p && *p != ' ') p++;
    }
  }  
}

static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
  int parms[4];
  parseParms(configString,parms);

  return new CSurf_USRT(parms[0],parms[1],parms[2],parms[3],errStats);
}


static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      {
        int parms[4];
        parseParms((const char *)lParam,parms);

        int n=GetNumMIDIInputs();
        int x=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"None");
        SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,x,-1);
        x=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)"None");
        SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,x,-1);
        for (x = 0; x < n; x ++)
        {
          char buf[512];
          if (GetMIDIInputName(x,buf,sizeof(buf)))
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)buf);
            SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,a,x);
            if (x == parms[2]) SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,a,0);
          }
        }
        n=GetNumMIDIOutputs();
        for (x = 0; x < n; x ++)
        {
          char buf[512];
          if (GetMIDIOutputName(x,buf,sizeof(buf)))
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)buf);
            SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,a,x);
            if (x == parms[3]) SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETCURSEL,a,0);
          }
        }
        SetDlgItemInt(hwndDlg,IDC_EDIT1,parms[0],TRUE);
        SetDlgItemInt(hwndDlg,IDC_EDIT2,parms[1],FALSE);
      }
    break;
    case WM_USER+1024:
      if (wParam > 1 && lParam)
      {
        char tmp[512];

        int indev=-1, outdev=-1, offs=0, size=9;
        int r=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
        if (r != CB_ERR) indev = SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETITEMDATA,r,0);
        r=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
        if (r != CB_ERR)  outdev = SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETITEMDATA,r,0);

        BOOL t;
        r=GetDlgItemInt(hwndDlg,IDC_EDIT1,&t,TRUE);
        if (t) offs=r;
        r=GetDlgItemInt(hwndDlg,IDC_EDIT2,&t,FALSE);
        if (t) 
        {
          if (r<1)r=1;
          else if(r>256)r=256;
          size=r;
        }

        sprintf(tmp,"%d %d %d %d",offs,size,indev,outdev);
        lstrcpyn((char *)lParam, tmp,wParam);
        
      }
    break;
  }
  return 0;
}

static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
  return CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_SURFACEEDIT_MCU),parent,dlgProc,(LPARAM)initConfigString);
}


reaper_csurf_reg_t csurf_usrt_reg =
{
  "USRT",
  "Ultraschall Midi Controller",
  createFunc,
  configFunc,
};
