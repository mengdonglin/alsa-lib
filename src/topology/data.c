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

/* Get Private data from a file. */
static int tplg_parse_data_file(snd_config_t *cfg, struct tplg_elem *elem)
{
	struct snd_soc_tplg_private *priv = NULL;
	const char *value = NULL;
	FILE *fp;
	size_t size;
	size_t bytes_read;
	int err = 0;

	tplg_dbg("data DataFile: %s\n", elem->id);

	if (snd_config_get_string(cfg, &value) < 0)
		return -EINVAL;

	fp = fopen(value, "r");
	if (fp == NULL) {
		fprintf(stderr, "Invalid Data file path '%s'\n", value);
		err = -errno;
		goto __err;
	}

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	if (size <= 0) {
		fprintf(stderr, "Invalid Data file size %zu\n", size);
		err = -EINVAL;
		goto __err;
	}

	priv = calloc(1, sizeof(*priv) + size);
	if (!priv) {
		err = -ENOMEM;
		goto __err;
	}

	bytes_read = fread(&priv->data, 1, size, fp);
	if (bytes_read != size) {
		err = -errno;
		goto __err;
	}

	elem->data = priv;
	priv->size = size;
	elem->size = sizeof(*priv) + size;
	return 0;

__err:
	if (priv)
		free(priv);
	return err;
}

static void dump_priv_data(struct tplg_elem *elem)
{
	struct snd_soc_tplg_private *priv = elem->data;
	unsigned char *p = (unsigned char *)priv->data;
	unsigned int i, j = 0;

	tplg_dbg(" elem size = %d, priv data size = %d\n",
		elem->size, priv->size);

	for (i = 0; i < priv->size; i++) {
		if (j++ % 8 == 0)
			tplg_dbg("\n");

		tplg_dbg(" 0x%x", *p++);
	}

	tplg_dbg("\n\n");
}

static int get_hex_num(const char *str)
{
	char *tmp, *s = NULL;
	int i = 0;

	tmp = strdup(str);
	if (tmp == NULL)
		return -ENOMEM;

	s = strtok(tmp, ",");
	while (s != NULL) {
		s = strtok(NULL, ",");
		i++;
	}

	free(tmp);
	return i;
}

static int write_hex(char *buf, char *str, int width)
{
	long val;
	void *p = &val;
	
        errno = 0;
	val = strtol(str, NULL, 16);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
		|| (errno != 0 && val == 0)) {
		return -EINVAL;
        }

	switch (width) {
	case 1:
		*(unsigned char *)buf = *(unsigned char *)p;
		break;
	case 2:
		*(unsigned short *)buf = *(unsigned short *)p;
		break;
	case 4:
		*(unsigned int *)buf = *(unsigned int *)p;
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

static int copy_data_hex(char *data, int off, const char *str, int width)
{
	char *tmp, *s = NULL, *p = data;
	int ret;

	tmp = strdup(str);
	if (tmp == NULL)
		return -ENOMEM;

	p += off;
	s = strtok(tmp, ",");

	while (s != NULL) {
		ret = write_hex(p, s, width);
		if (ret < 0) {
			free(tmp);
			return ret;
		}

		s = strtok(NULL, ",");
		p += width;
	}

	free(tmp);
	return 0;
}

static int tplg_parse_data_hex(snd_config_t *cfg, struct tplg_elem *elem,
	int width)
{
	struct snd_soc_tplg_private *priv;
	const char *value = NULL;
	int size, esize, off, num;
	int ret;

	tplg_dbg(" data: %s\n", elem->id);

	if (snd_config_get_string(cfg, &value) < 0)
		return -EINVAL;

	num = get_hex_num(value);
	size = num * width;
	priv = elem->data;

	if (priv != NULL) {
		off = priv->size;
		esize = elem->size + size;
		priv = realloc(priv, esize);
	} else {
		off = 0;
		esize = sizeof(*priv) + size;
		priv = calloc(1, esize);
	}

	if (!priv)
		return -ENOMEM;	
	
	elem->data = priv;
	priv->size += size;
	elem->size = esize;

	ret = copy_data_hex(priv->data, off, value, width);
	
	dump_priv_data(elem);
	return ret;
}


/* Parse Private data.
 *
 * Object private data
 *
 * SectionData."data name" {
 * 
 *		DataFile <filename>
 *		bytes
 *		shorts
 *		words
 *	}
 */
int tplg_parse_data(snd_tplg_t *tplg, snd_config_t *cfg,
	void *private ATTRIBUTE_UNUSED)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id;
	int err = 0;
	struct tplg_elem *elem;

	elem = create_elem_common(tplg, cfg, PARSER_TYPE_DATA);
	if (!elem)
		return -ENOMEM;

	snd_config_for_each(i, next, cfg) {

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0) {
			continue;
		}

		if (strcmp(id, "file") == 0) {
			err = tplg_parse_data_file(n, elem);
			if (err < 0) {
				fprintf(stderr, "error: failed to parse data file");
				return err;
			}
			continue;
		}

		if (strcmp(id, "bytes") == 0) {
			err = tplg_parse_data_hex(n, elem, 1);
			if (err < 0) {
				fprintf(stderr, "error: failed to parse data bytes");
				return err;
			}
			continue;
		}

		if (strcmp(id, "shorts") == 0) {
			err = tplg_parse_data_hex(n, elem, 2);
			if (err < 0) {
				fprintf(stderr, "error: failed to parse data shorts");
				return err;
			}
			continue;
		}

		if (strcmp(id, "words") == 0) {
			err = tplg_parse_data_hex(n, elem, 4);
			if (err < 0) {
				fprintf(stderr, "error: failed to parse data words");
				return err;
			}
			continue;
		}
	}

	return err;
}

/* copy private data into the bytes extended control */
int copy_data(struct tplg_elem *elem, struct tplg_elem *ref)
{
	struct snd_soc_tplg_private *priv;
	int priv_data_size;

	if (!ref)
		return -EINVAL;

	tplg_dbg("Data '%s' used by '%s'\n", ref->id, elem->id);
	priv_data_size = ref->data->size;

	switch (elem->type) {
		case PARSER_TYPE_MIXER:
			elem->mixer_ctrl = realloc(elem->mixer_ctrl, elem->size + priv_data_size);
			if (!elem->mixer_ctrl)
				return -ENOMEM;
			priv = &elem->mixer_ctrl->priv;
			break;

		case PARSER_TYPE_ENUM:
			elem->enum_ctrl = realloc(elem->enum_ctrl, elem->size + priv_data_size);
			if (!elem->enum_ctrl)
				return -ENOMEM;
			priv = &elem->enum_ctrl->priv;
			break;

		case PARSER_TYPE_BYTES:
			elem->bytes_ext = realloc(elem->bytes_ext, elem->size + priv_data_size);
			if (!elem->bytes_ext)
				return -ENOMEM;
			priv = &elem->bytes_ext->priv;
			break;


		case PARSER_TYPE_DAPM_WIDGET:
			elem->widget = realloc(elem->widget, elem->size + priv_data_size);
			if (!elem->widget)
				return -ENOMEM;
			priv = &elem->widget->priv;
			break;

		default:
			fprintf(stderr, "elem '%s': type %d shall not have private data\n",
				elem->id, elem->type);
			return -EINVAL;
	}

	elem->size += priv_data_size;
	priv->size = priv_data_size;
	memcpy(priv->data, ref->data->data, priv_data_size);
	return 0;
}

