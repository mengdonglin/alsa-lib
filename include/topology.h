/*
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Copyright (C) 2015 Intel Corporation
 *
 */

#ifndef __ALSA_TOPOLOGY_H
#define __ALSA_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/* public API header */

/* needs clean up with private header */

struct soc_tplg_priv;

int snd_parse_conf(struct soc_tplg_priv *soc_tplg, const char *filename);
struct soc_tplg_priv *snd_socfw_new(const char *name, int verbose);
void snd_socfw_free(struct soc_tplg_priv *soc_tplg);

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_TOPOLOGY_H */
