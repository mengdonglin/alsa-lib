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

*/

#include "list.h"
#include "tplg_local.h"

int add_ref(struct tplg_elem *elem, int type, const char* id)
{
	struct tplg_ref *ref;

	ref = calloc(1, sizeof(*ref));
	if (!ref)
		return -ENOMEM;

	strncpy(ref->id, id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	ref->type = type;

	list_add_tail(&ref->list, &elem->ref_list);
	return 0;
}

void free_ref_list(struct list_head *base)
{
	struct list_head *pos, *npos;
	struct tplg_ref *ref;

	list_for_each_safe(pos, npos, base) {
		ref = list_entry(pos, struct tplg_ref, list);
		list_del(&ref->list);
		free(ref);
	}
}

struct tplg_elem *tplg_elem_new(void)
{
	struct tplg_elem *elem;

	elem = calloc(1, sizeof(*elem));
	if (!elem)
		return NULL;

	INIT_LIST_HEAD(&elem->ref_list);
	return elem;
}

void tplg_elem_free(struct tplg_elem *elem)
{
	free_ref_list(&elem->ref_list);

	/* free struct snd_tplg_ object,
	 * the union pointers share the same address
	 */
	if (elem->mixer_ctrl)
		free(elem->mixer_ctrl);

	free(elem);
}

void tplg_elem_free_list(struct list_head *base)
{
	struct list_head *pos, *npos;
	struct tplg_elem *elem;

	list_for_each_safe(pos, npos, base) {
		elem = list_entry(pos, struct tplg_elem, list);
		list_del(&elem->list);
		tplg_elem_free(elem);
	}
}

struct tplg_elem *tplg_elem_lookup(struct list_head *base, const char* id,
	unsigned int type)
{
	struct list_head *pos, *npos;
	struct tplg_elem *elem;

	list_for_each_safe(pos, npos, base) {

		elem = list_entry(pos, struct tplg_elem, list);

		if (!strcmp(elem->id, id) && elem->type == type)
			return elem;
	}

	return NULL;
}

struct tplg_elem* tplg_elem_new_common(snd_tplg_t *tplg,
	snd_config_t *cfg, enum parser_type type)
{
	struct tplg_elem *elem;
	const char *id;
	int obj_size = 0;
	void *obj;

	elem = tplg_elem_new();
	if (!elem)
		return NULL;

	snd_config_get_id(cfg, &id);
	strncpy(elem->id, id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);

	switch (type) {
	case PARSER_TYPE_DATA:
		list_add_tail(&elem->list, &tplg->pdata_list);
		break;
		
	case PARSER_TYPE_TEXT:
		list_add_tail(&elem->list, &tplg->text_list);
		break;
	
	case PARSER_TYPE_TLV:
		list_add_tail(&elem->list, &tplg->tlv_list);
		elem->size = sizeof(struct snd_soc_tplg_ctl_tlv);
		break;

	case PARSER_TYPE_BYTES:
		list_add_tail(&elem->list, &tplg->bytes_ext_list);
		obj_size = sizeof(struct snd_soc_tplg_bytes_control);
		break;

	case PARSER_TYPE_ENUM:
		list_add_tail(&elem->list, &tplg->enum_list);
		obj_size = sizeof(struct snd_soc_tplg_enum_control);
		break;
		
	case SND_SOC_TPLG_TYPE_MIXER:
		list_add_tail(&elem->list, &tplg->mixer_list);
		obj_size = sizeof(struct snd_soc_tplg_mixer_control);
		break;
		
	case PARSER_TYPE_DAPM_WIDGET:
		list_add_tail(&elem->list, &tplg->widget_list);
		obj_size = sizeof(struct snd_soc_tplg_dapm_widget);
		break;
		
	case PARSER_TYPE_STREAM_CONFIG:
		list_add_tail(&elem->list, &tplg->pcm_config_list);
		obj_size = sizeof(struct snd_soc_tplg_stream_config);
		break;
		
	case PARSER_TYPE_STREAM_CAPS:
		list_add_tail(&elem->list, &tplg->pcm_caps_list);
		obj_size = sizeof(struct snd_soc_tplg_stream_caps);
		break;
		
	case PARSER_TYPE_PCM:
		list_add_tail(&elem->list, &tplg->pcm_list);
		obj_size = sizeof(struct snd_soc_tplg_pcm_dai);
		break;
	
	case PARSER_TYPE_BE:
		list_add_tail(&elem->list, &tplg->be_list);
		obj_size = sizeof(struct snd_soc_tplg_pcm_dai);
		break;
		
	case PARSER_TYPE_CC:
		list_add_tail(&elem->list, &tplg->cc_list);
		obj_size = sizeof(struct snd_soc_tplg_pcm_dai);
		break;
		
	default:
		free(elem);
		return NULL;
	}

	if (obj_size > 0) {
		obj = calloc(1, obj_size);
		if (obj == NULL) {
			free(elem);
			return NULL;
		}
		
		elem->obj = obj;
		elem->size = obj_size;
	}

	elem->type = type;
	return elem;	
}

