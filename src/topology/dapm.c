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

/* mapping of widget text names to types */
static const struct map_elem widget_map[] = {
	{"input", SND_SOC_TPLG_DAPM_INPUT},
	{"output", SND_SOC_TPLG_DAPM_OUTPUT},
	{"mux", SND_SOC_TPLG_DAPM_MUX},
	{"mixer", SND_SOC_TPLG_DAPM_MIXER},
	{"pga", SND_SOC_TPLG_DAPM_PGA},
	{"out_drv", SND_SOC_TPLG_DAPM_OUT_DRV},
	{"adc", SND_SOC_TPLG_DAPM_ADC},
	{"dac", SND_SOC_TPLG_DAPM_DAC},
	{"switch", SND_SOC_TPLG_DAPM_SWITCH},
	{"pre", SND_SOC_TPLG_DAPM_PRE},
	{"post", SND_SOC_TPLG_DAPM_POST},
	{"aif_in", SND_SOC_TPLG_DAPM_AIF_IN},
	{"aif_out", SND_SOC_TPLG_DAPM_AIF_OUT},
	{"dai_in", SND_SOC_TPLG_DAPM_DAI_IN},
	{"dai_out", SND_SOC_TPLG_DAPM_DAI_OUT},
	{"dai_link", SND_SOC_TPLG_DAPM_DAI_LINK},
};

/* mapping of widget kcontrol text names to types */
static const struct map_elem widget_control_map[] = {
	{"volsw", SND_SOC_TPLG_DAPM_CTL_VOLSW},
	{"enum_double", SND_SOC_TPLG_DAPM_CTL_ENUM_DOUBLE},
	{"enum_virt", SND_SOC_TPLG_DAPM_CTL_ENUM_VIRT},
	{"enum_value", SND_SOC_TPLG_DAPM_CTL_ENUM_VALUE},
};

static int lookup_widget(const char *w)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(widget_map); i++) {
		if (strcmp(widget_map[i].name, w) == 0)
			return widget_map[i].id;
	}

	return -EINVAL;
}

static int tplg_parse_dapm_mixers(snd_config_t *cfg, struct tplg_elem *elem)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *value = NULL;

	tplg_dbg(" DAPM Mixer Controls: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);

		/* get value */
		if (snd_config_get_string(n, &value) < 0)
			continue;

		tplg_ref_add(elem, PARSER_TYPE_MIXER, value);
		tplg_dbg("\t\t %s\n", value);
	}

	return 0;
}

static int tplg_parse_dapm_enums(snd_config_t *cfg, struct tplg_elem *elem)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *value = NULL;

	tplg_dbg(" DAPM Enum Controls: %s\n", elem->id);

	snd_config_for_each(i, next, cfg) {
		n = snd_config_iterator_entry(i);

		/* get value */
		if (snd_config_get_string(n, &value) < 0)
			continue;

		tplg_ref_add(elem, PARSER_TYPE_ENUM, value);
		tplg_dbg("\t\t %s\n", value);
	}

	return 0;
}

/* move referenced controls to the widget */
static int copy_dapm_control(struct tplg_elem *elem, struct tplg_elem *ref)
{
	struct snd_soc_tplg_dapm_widget *widget = elem->widget;
	struct snd_soc_tplg_mixer_control *mixer_ctrl = ref->mixer_ctrl;
	struct snd_soc_tplg_enum_control *enum_ctrl = ref->enum_ctrl;

	tplg_dbg("Control '%s' used by '%s'\n", ref->id, elem->id);
	tplg_dbg("\tparent size: %d + %d -> %d, priv size -> %d\n",
		elem->size, ref->size, elem->size + ref->size,
		widget->priv.size);

	widget = realloc(widget, elem->size + ref->size);
	if (!widget)
		return -ENOMEM;

	elem->widget = widget;

	/* copy new widget at the end */
	if (ref->type == PARSER_TYPE_MIXER)
		memcpy((void*)widget + elem->size, mixer_ctrl, ref->size);
	else if (ref->type == PARSER_TYPE_ENUM)
		memcpy((void*)widget + elem->size, enum_ctrl, ref->size);

	elem->size += ref->size;
	widget->num_kcontrols++;
	ref->compound_elem = 1;
	return 0;
}

/* check referenced controls for a widget */
static int tplg_check_widget(snd_tplg_t *tplg,
	struct tplg_elem *elem)
{
	struct tplg_ref *ref;
	struct list_head *base, *pos, *npos;
	int err = 0;

	base = &elem->ref_list;

	/* for each ref in this control elem */
	list_for_each_safe(pos, npos, base) {

		ref = list_entry(pos, struct tplg_ref, list);
		if (ref->id == NULL || ref->elem)
			continue;

		switch (ref->type) {
		case PARSER_TYPE_MIXER:
			ref->elem = tplg_elem_lookup(&tplg->mixer_list,
						ref->id, PARSER_TYPE_MIXER);
			if (ref->elem)
				err = copy_dapm_control(elem, ref->elem);
			break;

		case PARSER_TYPE_ENUM:
			ref->elem = tplg_elem_lookup(&tplg->enum_list,
						ref->id, PARSER_TYPE_ENUM);
			if (ref->elem)
				err = copy_dapm_control(elem, ref->elem);
			break;

		case PARSER_TYPE_DATA:
			ref->elem = tplg_elem_lookup(&tplg->pdata_list,
						ref->id, PARSER_TYPE_DATA);
			if (ref->elem)
				err = tplg_copy_data(elem, ref->elem);
			break;
		default:
			break;
		}

		if (!ref->elem) {
			fprintf(stderr, "error: cannot find control '%s'"
				" referenced by widget '%s'\n",
				ref->id, elem->id);
			return -EINVAL;
		}

		if (err < 0) 
			return err;
	}

	return 0;
}

int tplg_check_widgets(snd_tplg_t *tplg)
{

	struct list_head *base, *pos, *npos;
	struct tplg_elem *elem;
	int err;

	base = &tplg->widget_list;
	list_for_each_safe(pos, npos, base) {

		elem = list_entry(pos, struct tplg_elem, list);
		if (!elem->widget || elem->type != PARSER_TYPE_DAPM_WIDGET) {
			fprintf(stderr, "error: invalid widget '%s'\n",
				elem->id);
			return -EINVAL;
		}

		err = tplg_check_widget(tplg, elem);
		if (err < 0)
			return err;
	}

	return 0;
}

int tplg_check_routes(snd_tplg_t *tplg)
{
	struct list_head *base, *pos, *npos;
	struct tplg_elem *elem;
	struct snd_soc_tplg_dapm_graph_elem *route;

	base = &tplg->route_list;

	list_for_each_safe(pos, npos, base) {
		elem = list_entry(pos, struct tplg_elem, list);

		if (!elem->route || elem->type != PARSER_TYPE_DAPM_GRAPH) {
			fprintf(stderr, "error: invalid route '%s'\n",
				elem->id);
			return -EINVAL;
		}

		route = elem->route;
		tplg_dbg("\nCheck route: sink '%s', control '%s', source '%s'\n",
			route->sink, route->control, route->source);

		/* validate sink */
		if (strlen(route->sink) <= 0) {
			fprintf(stderr, "error: no sink\n");
			return -EINVAL;

		}
		if (!tplg_elem_lookup(&tplg->widget_list, route->sink,
			PARSER_TYPE_DAPM_WIDGET)) {
			fprintf(stderr, "error: undefined sink widget/stream '%s'\n",
				route->sink);
			return -EINVAL;
		}

		/* validate control name */
		if (strlen(route->control)) {
			if (!tplg_elem_lookup(&tplg->mixer_list,
				route->control, PARSER_TYPE_MIXER) &&
			!tplg_elem_lookup(&tplg->enum_list,
				route->control, PARSER_TYPE_ENUM)) {
				fprintf(stderr, "error: Undefined mixer/enum control '%s'\n",
					route->control);
			return -EINVAL;
			}
		}

		/* validate source */
		if (strlen(route->source) <= 0) {
			fprintf(stderr, "error: no source\n");
			return -EINVAL;

		}
		if (!tplg_elem_lookup(&tplg->widget_list, route->source,
			PARSER_TYPE_DAPM_WIDGET)) {
			fprintf(stderr, "error: Undefined source widget/stream '%s'\n",
				route->source);
			return -EINVAL;
		}
	}

	return 0;
}

#define LINE_SIZE	1024

/* line is defined as '"source, control, sink"' */
static int tplg_parse_line(const char *text,
	struct snd_soc_tplg_dapm_graph_elem *line)
{
	char buf[LINE_SIZE];
	unsigned int len, i;
	const char *source = NULL, *sink = NULL, *control = NULL;

	strncpy(buf, text, LINE_SIZE);

	len = strlen(buf);
	if (len <= 2) {
		fprintf(stderr, "error: invalid route \"%s\"\n", buf);
		return -EINVAL;
	}

	/* find first , */
	for (i = 1; i < len; i++) {
		if (buf[i] == ',')
			goto second;
	}
	fprintf(stderr, "error: invalid route \"%s\"\n", buf);
	return -EINVAL;

second:
	/* find second , */
	sink = buf;
	control = &buf[i + 2];
	buf[i] = 0;

	for (; i < len; i++) {
		if (buf[i] == ',')
			goto done;
	}

	fprintf(stderr, "error: invalid route \"%s\"\n", buf);
	return -EINVAL;

done:
	buf[i] = 0;
	source = &buf[i + 2];

	strcpy(line->source, source);
	strcpy(line->control, control);
	strcpy(line->sink, sink);
	return 0;
}


static int tplg_parse_routes(snd_tplg_t *tplg, snd_config_t *cfg)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	struct tplg_elem *elem;
	struct snd_soc_tplg_dapm_graph_elem *line = NULL;
	int err;

	snd_config_for_each(i, next, cfg) {
		const char *val;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_string(n, &val) < 0)
			continue;

		elem = tplg_elem_new();
		if (!elem)
			return -ENOMEM;

		list_add_tail(&elem->list, &tplg->route_list);
		strcpy(elem->id, "line");
		elem->type = PARSER_TYPE_DAPM_GRAPH;
		elem->size = sizeof(*line);

		line = calloc(1, sizeof(*line));
		if (!line)
			return -ENOMEM;

		elem->route = line;

		err = tplg_parse_line(val, line);
		if (err < 0)
			return err;

		tplg_dbg("route: sink '%s', control '%s', source '%s'\n",
				line->sink, line->control, line->source);
	}

	return 0;
}

int tplg_parse_dapm_graph(snd_tplg_t *tplg, snd_config_t *cfg,
	void *private ATTRIBUTE_UNUSED)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	int err;
	const char *graph_id;

	if (snd_config_get_type(cfg) != SND_CONFIG_TYPE_COMPOUND) {
		fprintf(stderr, "error: compound is expected for dapm graph definition\n");
		return -EINVAL;
	}

	snd_config_get_id(cfg, &graph_id);

	snd_config_for_each(i, next, cfg) {
		const char *id;

		n = snd_config_iterator_entry(i);
		if (snd_config_get_id(n, &id) < 0) {
			continue;
		}

		if (strcmp(id, "lines") == 0) {
			err = tplg_parse_routes(tplg, n);
			if (err < 0) {
				fprintf(stderr, "error: failed to parse dapm graph %s\n",
					graph_id);
				return err;
			}
			continue;
		}
	}

	return 0;
}

/* DAPM Widget.
 *
 * SectionWidget."widget name" {
 *
 *	index "1"
 *	type "aif_in"
 *	no_pm "true"
 *	shift "0"
 *	invert "1
 *	mixer "name"
 *	enum "name"
 *	data "name"
 * }
 */
int tplg_parse_dapm_widget(snd_tplg_t *tplg,
	snd_config_t *cfg, void *private ATTRIBUTE_UNUSED)
{
	struct snd_soc_tplg_dapm_widget *widget;
	struct tplg_elem *elem;
	snd_config_iterator_t i, next;
	snd_config_t *n;
	const char *id, *val = NULL;
	int widget_type, err;

	elem = tplg_elem_new_common(tplg, cfg, PARSER_TYPE_DAPM_WIDGET);
	if (!elem)
		return -ENOMEM;

	tplg_dbg(" Widget: %s\n", elem->id);

	widget = elem->widget;
	strncpy(widget->name, elem->id, SNDRV_CTL_ELEM_ID_NAME_MAXLEN);
	widget->size = elem->size;

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

		if (strcmp(id, "type") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			widget_type = lookup_widget(val);
			if (widget_type < 0){
				fprintf(stderr, "Widget '%s': Unsupported widget type %s\n",
					elem->id, val);
				return -EINVAL;
			}

			widget->id = widget_type;
			tplg_dbg("\t%s: %s\n", id, val);
			continue;
		}

		if (strcmp(id, "no_pm") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			if (strcmp(val, "true") == 0)
				widget->reg = -1;

			tplg_dbg("\t%s: %s\n", id, val);
			continue;
		}

		if (strcmp(id, "shift") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			widget->shift = atoi(val);
			tplg_dbg("\t%s: %d\n", id, widget->shift);
			continue;
		}

		if (strcmp(id, "invert") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			widget->invert = atoi(val);
			tplg_dbg("\t%s: %d\n", id, widget->invert);
			continue;
		}

		if (strcmp(id, "subseq") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			widget->subseq= atoi(val);
			tplg_dbg("\t%s: %d\n", id, widget->subseq);
			continue;
		}

		if (strcmp(id, "event_type") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			widget->event_type = atoi(val);
			tplg_dbg("\t%s: %d\n", id, widget->event_type);
			continue;
		}

		if (strcmp(id, "event_flags") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			widget->event_flags = atoi(val);
			tplg_dbg("\t%s: %d\n", id, widget->event_flags);
			continue;
		}

		if (strcmp(id, "enum") == 0) {
			err = tplg_parse_dapm_enums(n, elem);
			if (err < 0)
				return err;

			continue;			
		}

		if (strcmp(id, "mixer") == 0) {
			err = tplg_parse_dapm_mixers(n, elem);
			if (err < 0)
				return err;

			continue;		
		}

		if (strcmp(id, "data") == 0) {
			if (snd_config_get_string(n, &val) < 0)
				return -EINVAL;

			tplg_ref_add(elem, PARSER_TYPE_DATA, val);
			tplg_dbg("\t%s: %s\n", id, val);
			continue;
		}
	}

	return 0;
}
