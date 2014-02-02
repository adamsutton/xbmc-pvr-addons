/*
 *      Copyright (C) 2014 Adam Sutton
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "platform/threads/mutex.h"
#include "platform/util/timeutils.h"
#include "platform/sockets/tcp.h"

extern "C" {
#include "platform/util/atomic.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

#include "Tvheadend.h"
#include "client.h"

#define TVH_TO_DVD_TIME(x) ((double)x * DVD_TIME_BASE / 1000000.0)

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CHTSPDemuxer::CHTSPDemuxer ( CHTSPConnection &conn )
  : m_conn(conn), m_opened(false), m_started(false), m_chnId(0), m_subId(0)
{
}

CHTSPDemuxer::~CHTSPDemuxer ( void )
{
}

void CHTSPDemuxer::Reconnected ( void )
{
  /* Re-subscribe */
  if (m_opened) {
    tvhdebug("re-starting stream");
    SendSubscribe(true);
  }
}

/* **************************************************************************
 * Demuxer API
 * *************************************************************************/

bool CHTSPDemuxer::Open ( const PVR_CHANNEL &chn )
{
  /* Close current stream */
  Close();

  /* Cache data */
  m_chnId  = chn.iUniqueId;

  /* Open */
  m_started = false;
  m_opened  = SendSubscribe();
  return m_opened;
}

void CHTSPDemuxer::Close ( void )
{
  /* Nothing opened */
  if (!m_opened)
    return;
  m_opened  = false;
  m_started = false;
  
  /* Send unsubscribe */
  SendUnsubscribe();
}

DemuxPacket *CHTSPDemuxer::Read ( void )
{
  DemuxPacket *pkt;
  if (m_pktBuffer.Pop(pkt, 1000))
    return pkt;
  
  return PVR->AllocateDemuxPacket(0);
}

void CHTSPDemuxer::Flush ( void )
{
  DemuxPacket *pkt;
  while (m_pktBuffer.Pop(pkt))
    PVR->FreeDemuxPacket(pkt);
}

void CHTSPDemuxer::Abort ( void )
{
  m_streams.Clear();
}

int CHTSPDemuxer::CurrentId ( void )
{
  return -1;
}

PVR_ERROR CHTSPDemuxer::CurrentStreams ( PVR_STREAM_PROPERTIES *streams )
{
  CLockObject lock(m_mutex);
  if (!m_startCond.Wait(m_mutex, m_started, 5000))
    return PVR_ERROR_SERVER_ERROR;
  return m_streams.GetProperties(streams) ? PVR_ERROR_NO_ERROR
                                          : PVR_ERROR_SERVER_ERROR; 
}

PVR_ERROR CHTSPDemuxer::CurrentSignal ( PVR_SIGNAL_STATUS &sig )
{
  return PVR_ERROR_NO_ERROR;
}

/* **************************************************************************
 * Send Messages
 * *************************************************************************/

bool CHTSPDemuxer::SendSubscribe ( bool force )
{
  int err;
  htsmsg_t *m;

  /* Build message */
  m = htsmsg_create_map();
  htsmsg_add_s32(m, "channelId",       m_chnId);
  htsmsg_add_s32(m, "subscriptionId",  ++m_subId);
  htsmsg_add_u32(m, "timeshiftPeriod", (uint32_t)~0);

  /* Send and Wait for response */
  tvhdebug("subscribe to %08x", m_chnId);
  if (force)
    m = m_conn.SendAndWait0("subscribe", m);
  else
    m = m_conn.SendAndWait("subscribe", m);
  if (m == NULL)
  {
    tvhdebug("failed to send subscribe");
    return false;
  }

  /* Error */
  err = htsmsg_get_u32_or_default(m, "error", 0);
  htsmsg_destroy(m);
  if (err)
  {
    tvhdebug("failed to subscribe");
    return false;
  }

  tvhdebug("succesfully subscribed to %08x", m_chnId);
  return true;
}

void CHTSPDemuxer::SendUnsubscribe ( void )
{
  int err;
  htsmsg_t *m;
  
  /* Build message */
  m = htsmsg_create_map();
  htsmsg_add_s32(m, "subscriptionId", m_subId);

  /* Send and Wait */
  tvhdebug("unsubcribe from %d", m_subId);
  if ((m = m_conn.SendAndWait("unsubscribe", m)) == NULL)
  {
    tvhdebug("failed to send unsubcribe");
    return;
  }

  /* Error */
  err = htsmsg_get_u32_or_default(m, "error", 0);
  htsmsg_destroy(m);
  if (err)
  {
    tvhdebug("failed to unsubcribe");
    return;
  }

  tvhdebug("succesfully unsubscribed %d", m_subId);
}

/* **************************************************************************
 * Parse incoming data
 * *************************************************************************/

bool CHTSPDemuxer::ProcessMessage ( const char *method, htsmsg_t *m )
{
  uint32_t subId;

  /* No subscriptionId - not for demuxer */
  if (htsmsg_get_u32(m, "subscriptionId", &subId))
    return false;

  /* Not current subscription - ignore */
  else if (subId != m_subId)
    return true;

  /* Subscription messages */
  else if (!strcmp("muxpkt", method))
    ParseMuxPacket(m);
  else if (!strcmp("queueStatus", method))
    ParseQueueStatus(m);
  else if (!strcmp("signalStatus", method))
    ParseSignalStatus(m);
  else if (!strcmp("timeshiftStatus", method))
    ParseTimeshiftStatus(m);
  else if (!strcmp("subscriptionStart", method))
    ParseSubscriptionStart(m);
  else if (!strcmp("subscriptionStop", method))
    ParseSubscriptionStop(m);
  else if (!strcmp("subscriptionSkip", method))
    ParseSubscriptionSkip(m);
  else if (!strcmp("subscriptionSpeed", method))
    ParseSubscriptionSpeed(m);
  else
    tvhdebug("unhandled subscription message [%s]",
              method);

  return true;
}

void CHTSPDemuxer::ParseMuxPacket ( htsmsg_t *m )
{
  uint32_t    idx, u32;
  int64_t     s64;
  const void  *bin;
  size_t      binlen;
  DemuxPacket *pkt;

  /* Validate fields */
  if (htsmsg_get_u32(m, "stream", &idx) ||
      htsmsg_get_bin(m, "payload", &bin, &binlen))
  { 
    tvherror("malformed muxpkt");
    return;
  }

  /* Allocate buffer */
  if (!(pkt = PVR->AllocateDemuxPacket(binlen)))
    return;
  memcpy(pkt->pData, bin, binlen);
  pkt->iSize = binlen;

  /* Duration */
  if (!htsmsg_get_u32(m, "duration", &u32))
    pkt->duration = TVH_TO_DVD_TIME(u32);
  
  /* Timestamps */
  if (!htsmsg_get_s64(m, "dts", &s64))
    pkt->dts      = TVH_TO_DVD_TIME(s64);
  else
    pkt->dts      = DVD_NOPTS_VALUE;

  if (!htsmsg_get_s64(m, "pts", &s64))
    pkt->pts      = TVH_TO_DVD_TIME(s64);
  else
    pkt->pts      = DVD_NOPTS_VALUE;

  /* Find the stream */
  pkt->iStreamId = m_streams.GetStreamId(idx);

  /* Drop (could be done earlier) */
  if (pkt->iStreamId < 0)
  {
    PVR->FreeDemuxPacket(pkt);
    return;
  }

  /* Store */
  m_pktBuffer.Push(pkt);
}

void CHTSPDemuxer::ParseSubscriptionStart ( htsmsg_t *m )
{
  vector<XbmcPvrStream>  streams;
  htsmsg_t               *l;
  htsmsg_field_t         *f;
  DemuxPacket            *pkt;

  /* Validate */
  if ((l = htsmsg_get_list(m, "streams")) == NULL)
  {
    tvherror("malformed subscriptionStart");
    return;
  }

  /* Process each */
  HTSMSG_FOREACH(f, l)
  {
    uint32_t      idx, u32;
    const char    *type, *str;
    XbmcPvrStream stream;

    if (f->hmf_type != HMF_MAP)
      continue;
    if ((type = htsmsg_get_str(&f->hmf_msg, "type")) == NULL)
      continue;
    if (htsmsg_get_u32(&f->hmf_msg, "index", &idx))
      continue;

    /* Find stream */
    m_streams.GetStreamData(idx, &stream);
    CodecDescriptor codec = CodecDescriptor::GetCodecByName(type);
    if (codec.Codec().codec_type != XBMC_CODEC_TYPE_UNKNOWN)
    {
      stream.iPhysicalId = idx;
      stream.iCodecType  = codec.Codec().codec_type;
      stream.iCodecId    = codec.Codec().codec_id;

      /* Subtitle ID */
      if ((stream.iCodecType == XBMC_CODEC_TYPE_SUBTITLE) &&
          !strcmp("DVBSUB", type))
      {
        uint32_t composition_id = 0, ancillary_id = 0;
        htsmsg_get_u32(&f->hmf_msg, "composition_id", &composition_id);
        htsmsg_get_u32(&f->hmf_msg, "ancillary_id"  , &ancillary_id);
        stream.iIdentifier = (composition_id & 0xffff)
                           | ((ancillary_id & 0xffff) << 16);
      }

      /* Language */
      if (stream.iCodecType == XBMC_CODEC_TYPE_SUBTITLE ||
          stream.iCodecType == XBMC_CODEC_TYPE_AUDIO)
      {
        if ((str = htsmsg_get_str(&f->hmf_msg, "language")) != NULL)
          strncpy(stream.strLanguage, str, sizeof(stream.strLanguage));
      }

      /* Audio data */
      if (stream.iCodecType == XBMC_CODEC_TYPE_AUDIO)
      {
        stream.iChannels
          = htsmsg_get_u32_or_default(&f->hmf_msg, "channels", 0);
        stream.iSampleRate
          = htsmsg_get_u32_or_default(&f->hmf_msg, "rate", 0);
      }

      /* Video */
      if (stream.iCodecType == XBMC_CODEC_TYPE_VIDEO)
      {
        stream.iWidth   = htsmsg_get_u32_or_default(&f->hmf_msg, "width", 0);
        stream.iHeight  = htsmsg_get_u32_or_default(&f->hmf_msg, "height", 0);
        if ((u32 = htsmsg_get_u32_or_default(&f->hmf_msg, "aspect_den", 0)))
          stream.fAspect
            = (float)htsmsg_get_u32_or_default(&f->hmf_msg, "aspect_num", 0)
            / u32;
        else
          stream.fAspect = 0.0;
        if ((u32 = htsmsg_get_u32_or_default(&f->hmf_msg, "duration", 0)) > 0)
        {
          stream.iFPSScale = u32;
          stream.iFPSRate  = DVD_TIME_BASE;
        }
      }
        
      streams.push_back(stream);
      tvhdebug("id: %d, type %s, codec: %u",
                idx, codec.Name().c_str(), codec.Codec().codec_id);
    }
  }

  CLockObject lock(m_mutex);

  /* Update streams */
  m_streams.UpdateStreams(streams);
  pkt = PVR->AllocateDemuxPacket(0);
  pkt->iStreamId = DMX_SPECIALID_STREAMCHANGE;
  m_pktBuffer.Push(pkt);

  /* Source data */
  ParseSourceInfo(m);

  /* Signal */
  m_started = true;
  m_startCond.Broadcast();
}

void CHTSPDemuxer::ParseSourceInfo ( htsmsg_t *m )
{
}

void CHTSPDemuxer::ParseSubscriptionStop ( htsmsg_t *m )
{
  CLockObject lock(m_mutex);
  Flush();
  Abort();

  /* Reset signal */

  /* Reset source */
}

void CHTSPDemuxer::ParseSubscriptionSkip ( htsmsg_t *m )
{
}

void CHTSPDemuxer::ParseSubscriptionSpeed ( htsmsg_t *m )
{
}

void CHTSPDemuxer::ParseQueueStatus ( htsmsg_t *m )
{
}

void CHTSPDemuxer::ParseSignalStatus ( htsmsg_t *m )
{
}

void CHTSPDemuxer::ParseTimeshiftStatus ( htsmsg_t *m )
{
}
