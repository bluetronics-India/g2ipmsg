/*
 *  Copyright (C) 2006 Takeharu KATO
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "common.h"

gboolean can_sound=FALSE;

#if defined(HAVE_GST)
GstElement *pipeline, *source, *parser, *decoder, *conv, *sink;

static gboolean
on_bus_error_callback (GstBus     *bus,
		       GstMessage *message,
		       gpointer    data){
  GError *err;
  gchar *debug;

  gst_message_parse_error (message, &err, &debug);
  g_free (debug);

  dbg_out ("Error: %s\n", err->message);
  g_error_free (err);

  return TRUE;
}
static gboolean
on_bus_eos_callback (GstBus     *bus,
		     GstMessage *message,
		     gpointer    data){
  dbg_out ("eos\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  return TRUE;
}

static void
new_pad (GstElement *element,
	 GstPad     *pad,
	 gpointer    data)
{
  GstPad *sinkpad;
  /* We can now link this pad with the audio decoder */
  dbg_out ("Dynamic pad created, linking parser/decoder\n");

  sinkpad = gst_element_get_pad (decoder, "sink");
  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

void
play_sound(void){
  gchar *pathname;

  if ( (!can_sound) || (hostinfo_refer_ipmsg_default_sound()) )
    return;

  /* set filename property on the file source. Also add a message
   * handler. 
   */
  pathname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_SOUND,
                                        G2IPMSG_SOUND_FILE, TRUE, NULL);
  if (!pathname) {
    err_out("No sound file!!\n");
    return;
  }
  dbg_out("Sound file:%s\n",pathname);
  g_object_set (G_OBJECT (source), "location", pathname, NULL);
  g_free(pathname);

  /* Now set to playing and iterate. */
  dbg_out ("Setting to PLAYING\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}
void
init_sound_system(const char *appname){
  GstBus     *bus;

  gst_init (NULL, NULL);

  g_assert(appname);
  pipeline = gst_pipeline_new (appname);
  source = gst_element_factory_make ("filesrc", "file-source");
  parser = gst_element_factory_make ("oggdemux", "ogg-parser");
  decoder = gst_element_factory_make ("vorbisdec", "vorbis-decoder");
  conv = gst_element_factory_make ("audioconvert", "converter");

  sink = gst_element_factory_make ("gconfaudiosink", "sink");
  if (!GST_IS_ELEMENT (sink)) 
    sink = gst_element_factory_make ("alsasink", "alsa-output");
  if (!GST_IS_ELEMENT (sink)) {
    err_out ("Could not get default audio sink from GConf\n");
    return;
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message::error", G_CALLBACK (on_bus_error_callback), NULL);
  g_signal_connect (bus, "message::eos", G_CALLBACK (on_bus_eos_callback), NULL);

  /* put all elements in a bin */
  gst_bin_add_many (GST_BIN (pipeline),
		    source, parser, decoder, conv, sink, NULL);

  /* link together - note that we cannot link the parser and
   * decoder yet, becuse the parser uses dynamic pads. For that,
   * we set a pad-added signal handler. */
  gst_element_link (source, parser);
  gst_element_link_many (decoder, conv, sink, NULL);
  g_signal_connect (parser, "pad-added", G_CALLBACK (new_pad), NULL);
  can_sound=TRUE;
  gst_element_set_state (pipeline, GST_STATE_READY);
  play_sound();

  return ;  
}
void
cleanup_sound_system(void){
  if (GST_IS_PIPELINE (pipeline))
    return;

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
}
#else
void 
init_sound_system(const char *name){
  can_sound=FALSE;
  return;
}
void 
play_sound(void) {
  return;
}
void 
cleanup_sound_system(void){
  return;
}

#endif
