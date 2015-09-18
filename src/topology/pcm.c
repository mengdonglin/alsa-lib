/*
  Copyright(c) 2014-2015 Intel Corporation
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  Authors: Mengdong Lin <mengdong.lin@intel.com>
           Yao Jin <yao.jin@intel.com>
           Liam Girdwood <liam.r.girdwood@linux.intel.com>
*/

#include "list.h"
#include "tplg_local.h"

struct tplg_elem *lookup_pcm_dai_stream(struct list_head *base, const char* id)
{
	struct list_head *pos;
	struct tplg_elem *elem;
	struct snd_soc_tplg_pcm *pcm_dai;

	list_for_each(pos, base) {

		elem = list_entry(pos, struct tplg_elem, list);
		if (elem->type != SND_TPLG_TYPE_PCM)
			return NULL;

		pcm_dai = elem->pcm;

		if (pcm_dai && !strcmp(pcm_dai->dai_name, id))
			return elem;
	}

	return NULL;
}

/* copy referenced caps to the pcm */
static void copy_pcm_caps(const char *id, struct snd_soc_tplg_stream_caps *caps,
	struct tplg_elem *ref_elem)
{
	struct snd_soc_tplg_stream_caps *ref_caps = ref_elem->stream_caps;

	tplg_dbg("Copy pcm caps (%ld bytes) from '%s' to '%s' \n",
		sizeof(*caps), ref_elem->id, id);

	*caps =  *ref_caps;
}

/* copy referenced config to the pcm */
static void copy_pcm_config(const char *id,
	struct snd_soc_tplg_stream *cfg, struct tplg_elem *ref_elem)
{
	struct snd_soc_tplg_stream *ref_cfg = ref_elem->stream_cfg;

	tplg_dbg("Copy pcm config (%ld bytes) from '%s' to '%s' \n",
		sizeof(*cfg), ref_elem->id, id);

	*cfg = *ref_cfg;
}

/* check referenced config and caps for a pcm */
static int tplg_build_pcm_caps(snd_tplg_t *tplg, struct tplg_elem *elem)
{
	struct tplg_elem *ref_elem = NULL;
	struct snd_soc_tplg_pcm *pcm_dai;
	struct snd_soc_tplg_stream_caps *caps;
	struct snd_soc_tplg_stream *stream;
	unsigned int i;

	pcm_dai = elem->pcm;

	for (i = 0; i < 2; i++) {
		caps = &pcm_dai->caps[i];

		ref_elem = tplg_elem_lookup(&tplg->pcm_caps_list,
			caps->name, SND_TPLG_TYPE_STREAM_CAPS);

		if (ref_elem != NULL)
			copy_pcm_caps(elem->id, caps, ref_elem);
	}

	for (i = 0; i < pcm_dai->num_streams; i++) {
		stream = &pcm_dai->stream[i];

		ref_elem = tplg_elem_lookup(&tplg->pcm_config_list,
			stream->name,
			SND_TPLG_TYPE_STREAM_CONFIG);

		if (ref_elem != NULL)
			copy_pcm_config(elem->id,
				stream, ref_elem);
	}

	return 0;
}

/* build FE DAI/PCM configurations */
int tplg_build_pcm(snd_tplg_t *tplg, unsigned int type)
{
	struct list_head *base, *pos;
	struct tplg_elem *elem;
	int err = 0;

	base = &tplg->pcm_list;
	list_for_each(pos, base) {

		elem = list_entry(pos, struct tplg_elem, list);
		if (elem->type != type) {
			SNDERR("error: invalid elem '%s'\n", elem->id);
			return -EINVAL;
		}

		err = tplg_build_pcm_caps(tplg, elem);
		if (err < 0)
			return err;
	}

	return 0;
}

/* build BE/CC DAI link configurations */
int tplg_build_link_cfg(snd_tplg_t *tplg, unsigned int type)
{
	struct list_head *base, *pos;
	struct tplg_elem *elem;
	int err = 0;

	switch (type) {
	case SND_TPLG_TYPE_BE:
		base = &tplg->be_list;
		break;
	case SND_TPLG_TYPE_CC:
		base = &tplg->cc_list;
		break;
	default:
		return -EINVAL;
	}

	list_for_each(pos, base) {

		elem = list_entry(pos, struct tplg_elem, list);
		if (elem->type != type) {
			SNDERR("error: invalid elem '%s'\n", elem->id);
			return -EINVAL;
		}

		/*TODO: export link configurations */
		//err = tplg_build_stream_cfg(tplg, elem);
		if (err < 0)
			return err;
	}

	return 0;
}

/* PCM stream configuration
 * TODO: change the BDW text format: no need to seperate playback/capture
 */
static int tplg_parse_stream_cfg(snd_tplg_t *tplg ATTRIBUTE_UNUSED,
	snd_config_t *cfg, void *private)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	struct snd_soc_tplg_stream *stream = private;
	const char *id, *val;
	snd_pcm_format_t format;
	int ret;

	return 0;

	snd_config_get_id(cfg, &id);

	tplg_dbg("\t%s:\n", id);

	stream->size = sizeof(*stream);

	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);

		if (snd_config_get_id(n, &id) < 0)
			return -EINVAL;

		if (snd_config_get_string(n, &val) < 0)
			return -EINVAL;

		if (strcmp(id, "format") == 0) {
			format = snd_pcm_format_value(val);
			if (format == SND_PCM_FORMAT_UNKNOWN) {
				SNDERR("error: unsupported stream format %s\n",
					val);
				return -EINVAL;
			}

			stream->format = format;
			tplg_dbg("\t\t%s: %s\n", id, val);
			continue;
		}

		if (strcmp(id, "rate") == 0) {
			stream->rate = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, stream->rate);
			continue;
		}

		if (strcmp(id, "channels_min") == 0) {
			stream->channels_min = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, stream->channels_min);
			continue;
		}

		if (strcmp(id, "channels_max") == 0) {
			stream->channels_max = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, stream->channels_max);
			continue;
		}
	}

	return 0;
}

/* Parse pcm configuration */
int tplg_parse_pcm_config(snd_tplg_t *tplg,
	snd_config_t *cfg, void *private ATTRIBUTE_UNUSED)
{
	struct snd_soc_tplg_stream *sc;
	struct tplg_elem *elem;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id;
	int err;

	elem = tplg_elem_new_common(tplg, cfg, NULL, SND_TPLG_TYPE_STREAM_CONFIG);
	if (!elem)
		return -ENOMEM;

	sc = elem->stream_cfg;
	sc->size = elem->size;

	tplg_dbg(" PCM Config: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/* skip comments */
		if (strcmp(id, "comment") == 0)
			continue;
		if (id[0] == '#')
			continue;

		if (strcmp(id, "config") == 0) {
			err = tplg_parse_compound(tplg, n,
				tplg_parse_stream_cfg, sc);
			if (err < 0)
				return err;
			continue;
		}
	}

	return 0;
}

static int split_format(struct snd_soc_tplg_stream_caps *caps, char *str)
{
	char *s = NULL;
	snd_pcm_format_t format;
	int i = 0, ret;

	s = strtok(str, ",");
	while ((s != NULL) && (i < SND_SOC_TPLG_MAX_FORMATS)) {
		format = snd_pcm_format_value(s);
		if (format == SND_PCM_FORMAT_UNKNOWN) {
			SNDERR("error: unsupported stream format %s\n", s);
			return -EINVAL;
		}

		caps->formats |= 1 << format;
		s = strtok(NULL, ", ");
		i++;
	}

	return 0;
}

/* Parse pcm Capabilities */
int tplg_parse_pcm_caps(snd_tplg_t *tplg,
	snd_config_t *cfg, void *private ATTRIBUTE_UNUSED)
{
	struct snd_soc_tplg_stream_caps *sc;
	struct tplg_elem *elem;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id, *val;
	char *s;
	int err;

	elem = tplg_elem_new_common(tplg, cfg, NULL, SND_TPLG_TYPE_STREAM_CAPS);
	if (!elem)
		return -ENOMEM;

	sc = elem->stream_caps;
	sc->size = elem->size;
	elem_copy_text(sc->name, elem->id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	tplg_dbg(" PCM Capabilities: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/* skip comments */
		if (strcmp(id, "comment") == 0)
			continue;
		if (id[0] == '#')
			continue;

		if (snd_config_get_string(n, &val) < 0)
			return -EINVAL;

		if (strcmp(id, "formats") == 0) {
			s = strdup(val);
			if (s == NULL)
				return -ENOMEM;

			err = split_format(sc, s);
			free(s);

			if (err < 0)
				return err;

			tplg_dbg("\t\t%s: %s\n", id, val);
			continue;
		}

		if (strcmp(id, "rate_min") == 0) {
			sc->rate_min = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, sc->rate_min);
			continue;
		}

		if (strcmp(id, "rate_max") == 0) {
			sc->rate_max = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, sc->rate_max);
			continue;
		}

		if (strcmp(id, "channels_min") == 0) {
			sc->channels_min = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, sc->channels_min);
			continue;
		}

		if (strcmp(id, "channels_max") == 0) {
			sc->channels_max = atoi(val);
			tplg_dbg("\t\t%s: %d\n", id, sc->channels_max);
			continue;
		}
	}

	return 0;
}

static int tplg_parse_pcm_cfg(snd_tplg_t *tplg ATTRIBUTE_UNUSED,
	snd_config_t *cfg, void *private)
{
	struct snd_soc_tplg_pcm *pcm_dai = private;
	struct snd_soc_tplg_stream *streams = pcm_dai->stream;
	unsigned int *num_streams = &pcm_dai->num_streams;
	const char *value;

	if (*num_streams == SND_SOC_TPLG_STREAM_CONFIG_MAX)
		return -EINVAL;

	if (snd_config_get_string(cfg, &value) < 0)
		return EINVAL;

	elem_copy_text(streams[*num_streams].name, value,
		SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	*num_streams += 1;

	tplg_dbg("\t\t\t%s\n", value);

	return 0;
}

/* Parse the cap and config of a pcm */
int tplg_parse_dai_pcm_caps(snd_tplg_t *tplg, snd_config_t *cfg,
	void *private)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	struct tplg_elem *elem = private;
	struct snd_soc_tplg_pcm *pcm_dai;
	const char *id, *value;
	int err, stream;

	pcm_dai = elem->pcm;

	snd_config_get_id(cfg, &id);

	tplg_dbg("\t%s:\n", id);

	if (strcmp(id, "playback") == 0) {
		stream = SND_SOC_TPLG_STREAM_PLAYBACK;
		pcm_dai->playback = 1;
	} else if (strcmp(id, "capture") == 0) {
		stream = SND_SOC_TPLG_STREAM_CAPTURE;
		pcm_dai->capture = 1;
	} else
		return -EINVAL;

	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);

		/* get id */
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, "capabilities") == 0) {
			if (snd_config_get_string(n, &value) < 0)
				continue;

			elem_copy_text(pcm_dai->caps[stream].name, value,
				SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

			tplg_dbg("\t\t%s\n\t\t\t%s\n", id, value);
			continue;
		}
	}

	return 0;
}

/* Parse FE DAI/pcm */
int tplg_parse_pcm(snd_tplg_t *tplg,
	snd_config_t *cfg, void *private ATTRIBUTE_UNUSED)
{
	struct snd_soc_tplg_pcm *pcm_dai;
	struct tplg_elem *elem;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id, *val = NULL;
	int err;

	elem = tplg_elem_new_common(tplg, cfg, NULL, SND_TPLG_TYPE_PCM);
	if (!elem)
		return -ENOMEM;

	pcm_dai = elem->pcm;
	pcm_dai->size = elem->size;
	elem_copy_text(pcm_dai->dai_name, elem->id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	tplg_dbg(" PCM: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/* skip comments */
		if (strcmp(id, "comment") == 0)
			continue;
		if (id[0] == '#')
			continue;

		if (strcmp(id, "index") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			elem->index = atoi(val);
			tplg_dbg("\t%s: %d\n", id, elem->index);
			continue;
		}

		if (strcmp(id, "id") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			pcm_dai->dai_id = atoi(val);
			tplg_dbg("\t%s: %d\n", id, pcm_dai->dai_id);
			continue;
		}

		if (strcmp(id, "pcm") == 0) {
			err = tplg_parse_compound(tplg, n,
				tplg_parse_dai_pcm_caps, elem);
			if (err < 0)
				return err;
			continue;
		}

		/* TODO: udpate text file for PCM configures */
		if (strcmp(id, "configs") == 0) {
			tplg_dbg("\t\tconfigs:\n");
			err = tplg_parse_compound(tplg, n, tplg_parse_pcm_cfg,
				&pcm_dai);
			if (err < 0)
				return err;
			continue;
		}
	}

	return 0;
}

/* Parse the BE/Codec<->Codec link configurations */
int tplg_parse_link_cfg(snd_tplg_t *tplg,
	snd_config_t *cfg, 	void *private ATTRIBUTE_UNUSED)
{
	/* TODO: finish parsing with new text format
	 * replace tplg_parse_be & tplg_parse_cc
	 */
	return 0;
}

/* Parse be: TO BE replaced by  */
int tplg_parse_be(snd_tplg_t *tplg,
	snd_config_t *cfg, void *private ATTRIBUTE_UNUSED)
{
	struct snd_soc_tplg_link_config *link;
	struct tplg_elem *elem;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id, *val = NULL;
	int err;

	elem = tplg_elem_new_common(tplg, cfg, NULL, SND_TPLG_TYPE_BE);
	if (!elem)
		return -ENOMEM;

	link = elem->be;
	link->size = elem->size;
	elem_copy_text(link->name, elem->id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	tplg_dbg(" BE: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/* skip comments */
		if (strcmp(id, "comment") == 0)
			continue;
		if (id[0] == '#')
			continue;

		if (strcmp(id, "index") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			elem->index = atoi(val);
			tplg_dbg("\t%s: %d\n", id, elem->index);
			continue;
		}

		if (strcmp(id, "id") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			link->id = atoi(val);
			tplg_dbg("\t%s: %d\n", id, link->id);
			continue;
		}
	}

	return 0;
}

/* Parse cc */
int tplg_parse_cc(snd_tplg_t *tplg,
	snd_config_t *cfg, void *private ATTRIBUTE_UNUSED)
{
	struct snd_soc_tplg_link_config *link;
	struct tplg_elem *elem;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id, *val = NULL;
	int err;

	elem = tplg_elem_new_common(tplg, cfg, NULL, SND_TPLG_TYPE_CC);
	if (!elem)
		return -ENOMEM;

	link = elem->cc;
	link->size = elem->size;

	tplg_dbg(" CC: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0)
			continue;

		/* skip comments */
		if (strcmp(id, "comment") == 0)
			continue;
		if (id[0] == '#')
			continue;

		if (strcmp(id, "index") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			elem->index = atoi(val);
			tplg_dbg("\t%s: %d\n", id, elem->index);
			continue;
		}

		if (strcmp(id, "id") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			link->id = atoi(val);
			tplg_dbg("\t%s: %d\n", id, link->id);
			continue;
		}

	}

	return 0;
}

/* copy stream object */
static void tplg_add_stream_object(struct snd_soc_tplg_stream *to_link,
				struct snd_tplg_stream_template *from_link)
{
	elem_copy_text(to_link->name, from_link->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	to_link->format = from_link->format;
	to_link->rate = from_link->rate;
	to_link->period_bytes = from_link->period_bytes;
	to_link->buffer_bytes = from_link->buffer_bytes;
	to_link->channels_min = from_link->channels_min;
	to_link->channels_max = from_link->channels_max;
}

int tplg_add_link_object(snd_tplg_t *tplg, snd_tplg_obj_template_t *t)
{
	struct snd_tplg_link_template *link = t->link;
	struct snd_soc_tplg_link_config *lk;
	struct tplg_elem *elem;
	int i;

	/* here type can be either BE or CC. */
	elem = tplg_elem_new_common(tplg, NULL, link->name, t->type);

	if (!elem)
		return -ENOMEM;
	if (t->type == SND_TPLG_TYPE_BE) {
		tplg_dbg("BE Link: %s", link->name);
		lk = elem->be;
	}
	else if (t->type == SND_TPLG_TYPE_CC) {
		tplg_dbg("CC Link: %s", link->name);
		lk = elem->cc;
	}
	else {
		tplg_elem_free(elem);
		return -EINVAL;
	}

	lk->size = elem->size;
	lk->id = link->id;
	elem_copy_text(lk->name, link->name, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	lk->num_streams = link->num_streams;

	for (i=0; i < lk->num_streams; i++)
		tplg_add_stream_object(&lk->stream[i], &link->stream[i]);

	return 0;
}
