#!/usr/bin/env python
#
# This file is part of Cosmonium.
#
# Copyright (C) 2018-2019 Laurent Deru.
#
# Cosmonium is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Cosmonium is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Cosmonium.  If not, see <https://www.gnu.org/licenses/>.
#

from __future__ import print_function
from __future__ import absolute_import

import sys, os
import re

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from cosmonium.astro import units, astro

from cosmonium.astro.parsers.kostjukparser import KostjukParser
from cosmonium.astro.parsers.csnparser import CSNParser
from cosmonium.astro.parsers.ybsparser import YBSParser
from cosmonium.astro.parsers.soederhjelmparser import SoederhejlmParser
from cosmonium.astro.parsers.sb9parser import SB9Parser
from cosmonium.astro.parsers.orb6parser import ORB6Parser
from cosmonium.astro.parsers.wdsparser import WdsParser
from cosmonium.astro.parsers.hipparser import HipParser
from cosmonium.astro.parsers.glieseparser import GlieseParser

multiplicity = {'71683': {'second': '71681', 'names': ['ALF Cen', 'Gliese 559'],
                          '71681': { 'hip': '71681', 'hd': '128620', 'hr': '5459', 'bayer': 'ALF02 Cen'},
                          '71683': { 'hip': '71683', 'hd': '128621', 'hr': '5460', 'bayer': 'ALF01 Cen'},
                         }
               }

has_multiplicity = {}
for entry_id, entry in multiplicity.items():
    has_multiplicity[entry_id] = True
    has_multiplicity[entry['second']] = True

def collect_names(proper_names, data, comp=None, in_system=False):
    names = []
    hr = None
    gl = None
    for entry in data:
        if hr is None:
            hr = entry.get('hr', None)
        if gl is None:
            gl = entry.get('gl', None)
        if comp is None and gl is not None and gl != '':
            comp = entry.get('comp', '')
    hr_name = None
    #TODO: Empty catalogue reference should be None and not ''
    if hr is not None and hr != '':
        hr_name = 'HR ' + hr
        if hr_name in proper_names:
            proper_name = proper_names[hr_name]
            if in_system and comp is not None and comp != '':
                proper_name = proper_name + " " + comp
            names.append(proper_name)
    elif gl is not None and gl != '':
        gl_name = re.sub('Gl', 'GJ', gl)
        if gl_name in proper_names:
            proper_name = proper_names[gl_name]
            if in_system and comp is not None and comp != '':
                proper_name = proper_name + " " + comp
            names.append(proper_name)
    for entry in data:
        bayer = entry.get('bayer', '')
        if bayer != '':
            if in_system and comp is not None and comp != '':
                bayer = bayer + " " + comp
            names.append(bayer)
            break
    for entry in data:
        flamsteed = entry.get('flamsteed', '')
        if flamsteed != '':
            if in_system and comp is not None and comp != '':
                flamsteed = flamsteed + " " + comp
            names.append(flamsteed)
            break
    if comp == '' or not in_system:
        if hr_name is not None:
            names.append(hr_name)
    if gl is not None and gl != '':
        gl = re.sub('^(Gl|Gj|NN|Wo) ', r'Gliese ', gl)
        if comp != '':
            gl = gl + " " + comp
        names.append(gl)
    for entry in data:
        ads = entry.get('ads', '')
        if ads != '':
            if comp is not None and comp != '':
                ads = "ADS " + ads + " " + comp
            else:
                ads = "ADS " + ads + " AB" #TODO: get real list of components
            names.append(ads)
            break
    if comp == '' or not in_system:
        for entry in data:
            hip = entry.get('hip', None)
            if hip is not None and hip != '':
                if isinstance(hip, list):
                    print("Multiple HIP", hip)
                else:
                    hip_name = 'HIP ' + hip
                    names.append(hip_name)
                    break
        for entry in data:
            hd = entry.get('hd', None)
            if hd is not None and hd != '':
                if isinstance(hd, list):
                    print("Multiple entries for HD", hd)
                else:
                    hd_name = 'HD ' + hd
                    names.append(hd_name)
                    break
        for entry in data:
            sao = entry.get('sao', None)
            if sao is not None and hd != '':
                if isinstance(hd, list):
                    print("Multiple entries for SAO", sao)
                else:
                    sao_name = 'SAO ' + sao
                    names.append(sao_name)
                    break
    for entry in data:
        wds = entry.get('wds', '')
        if wds != '':
            if comp is not None and comp != '':
                wds = "WDS " + wds + " " + comp
            else:
                wds = "WDS " + wds + " AB" #TODO: get real list of components
            names.append(wds)
            break
    if len(names) == 0 and comp != '':
        names = [comp]
    return names

def add_star(proper_names, xref, entries):
    distance = 0
    spectral_type = ''
    vmag = None
    ra = ''
    de = ''
    for entry in entries:
        if distance <= 0 and entry['parallax'] is not None and entry['parallax'] > 0:
            distance = 1.0 / entry['parallax']
        if spectral_type == '':
            spectral_type = entry['spectral_type']
        if vmag is None:
            vmag = entry['vmag']
        if ra == '':
            ra = entry['ra']
        if de == '':
            de = entry['de']
    if distance <= 0:
        print("Invalid parallax for", entries)
        return ''
    try:
        abs_mag = astro.app_to_abs_mag(vmag, distance * units.Parsec)
    except ValueError as e:
        print("Invalid mag or distance", vmag, distance, e)
        return
    names_source = [xref] + entries
    names = collect_names(proper_names, names_source)
    result = """- star:
    name: [%s]
    orbit:
        type: fixed
        ra: %g
        de: %g
        distance: %g
        distance-units: pc
    magnitude: %g
    spectral-type: '%s'
""" % (', '.join(names), ra, de, distance, abs_mag, spectral_type)
    return result

def add_binary_system(proper_names, xref, system, stars, orbit):
    if system['parallax'] <= 0:
        print("Invalid parallax for", orbit['hip'])
        return ''
    distance = 1.0 / system['parallax']
    spectral_type = None
    spectral_type_1 = None
    spectral_type_2 = None
    app_mag_1 = None
    app_mag_2 = None
    for star in stars:
        if spectral_type_1 is None or spectral_type_2 is None:
            if star['src'] == 'sb9':
                if star['spectral_type_2'] != '':
                    spectral_type_1 = star['spectral_type_1']
                    spectral_type_2 = star['spectral_type_2']
                else:
                    spectral_type = star['spectral_type_1']
            elif star['src'] == 'wds':
                spectral_type = star['spectral_type']
                if '+' in spectral_type:
                    spectral_type_1, spectral_type_2 = spectral_type.split('+')
        if app_mag_1 is None or app_mag_2 is None:
            if star['src'] == 'sb9' or star['src'] == 'wds':
                app_mag_1 = star['app_mag_1']
                abs_mag_1 = astro.app_to_abs_mag(app_mag_1, distance * units.Parsec)
                app_mag_2 = star['app_mag_2']
                if app_mag_2 is not None:
                    abs_mag_2 = astro.app_to_abs_mag(app_mag_2, distance * units.Parsec)
            elif star['src'] == 'orb6':
                app_mag_1 = star['v1']
                abs_mag_1 = astro.app_to_abs_mag(app_mag_1, distance * units.Parsec)
                app_mag_2 = star['v2']
                if app_mag_2 is not None:
                    abs_mag_2 = astro.app_to_abs_mag(app_mag_2, distance * units.Parsec)
    if spectral_type_1 is None or spectral_type_2 is None:
        if spectral_type is None:
            spectral_type = system['spectral_type']
        if spectral_type is not None and spectral_type != '':
            spectral_type_1 = spectral_type
            spectral_type_2 = spectral_type
    if spectral_type_1 is None or spectral_type_2 is None:
        print("No spectral type for HIP", orbit['hip'])
        return ''
    if app_mag_1 is None or app_mag_2 is None:
        print("No Apparent magnitude for HIP", orbit['hip'])
        return ''
    if orbit['multiplicity'] == '':
        names = collect_names(proper_names, [xref, system, orbit] + stars, '')
        names_1 = collect_names(proper_names, [xref, system, orbit] + stars, 'A', True)
        names_2 = collect_names(proper_names, [xref, system, orbit] + stars, 'B', True)
    else:
        fix = multiplicity[orbit['hip']]
        hip_1 = orbit['hip']
        hip_2 = fix['second']
        names = fix['names']
        names_1 = collect_names(proper_names, [fix[hip_1]])
        names_2 = collect_names(proper_names, [fix[hip_2]])
    mass_sum = orbit['mass_sum']
    mass_ratio = orbit['mass_ratio']
    sma = orbit['semimajoraxis']
    period = orbit['period']
    eccentricity = orbit['eccentricity']
    inclination = orbit['inclination']
    ascending_node = orbit['pa_of_node']
    arg_of_periapsis = orbit['arg_of_periapsis']
    epoch = orbit['epoch']
    mass_1 = mass_sum / (1.0 + mass_ratio)
    mass_2 = mass_1 * mass_ratio
    sma_1 = units.arcsec_to_rad(sma / (1.0 + mass_ratio))
    sma_2 = sma_1 * mass_ratio
    sma_1 *= distance * units.Parsec / units.AU
    sma_2 *= distance * units.Parsec / units.AU
    arg_of_periapsis_1 = arg_of_periapsis - 180 if arg_of_periapsis >= 180 else arg_of_periapsis + 180
    arg_of_periapsis_2 = arg_of_periapsis
    epoch_j2000 = 2000.0 - epoch
    fract = epoch_j2000 / period
    mean_anomaly = (fract * 360) % 360
    result = """- barycenter:
    name: [%s]
    orbit:
        type: fixed
        ra: %g
        de: %g
        distance: %g
        distance-units: pc
    children:
"""% (', '.join(names), system['ra'], system['de'], distance)
    result += """      - star:
          name: [%s]
          magnitude: %g
          spectral-type: '%s'
          orbit:
            type: elliptic
            semi-major-axis: %g
            period: %g
            eccentricity: %g
            inclination: %g
            ascending-node: %g
            arg-of-periapsis: %g
            mean-anomaly: %g
""" % (', '.join(names_1), abs_mag_1, spectral_type_1,
       sma_1, period / units.JYear, eccentricity, inclination, ascending_node, arg_of_periapsis_1, mean_anomaly)
    result += """      - star:
          name: [%s]
          magnitude: %g
          spectral-type: '%s'
          orbit:
            type: elliptic
            semi-major-axis: %g
            period: %g
            eccentricity: %g
            inclination: %g
            ascending-node: %g
            arg-of-periapsis: %g
            mean-anomaly: %g
""" % (', '.join(names_2), abs_mag_2, spectral_type_2,
       sma_2, period / units.JYear, eccentricity, inclination, ascending_node, arg_of_periapsis_2, mean_anomaly)
    return result

KostjukCatalogue = 'data/Kostjuk/catalog.dat'
CSNCatalogue = 'data/IAU-CSN.txt'
HIPCatalogue = 'data/hip'
GlieseCatalogue = 'data/gliese/catalog.dat'
YBSCatalogue = 'data/ybs/bsc5.dat'
SoederhjelmCatalogue = 'data/Soederhjelm'
SB9Catalogue = 'data/sb9'
ORB6Catalogue = 'data/orb6/orb6orbits.sql.txt'
WDSCatalogue = 'data/wds'
StarsFile = 'stars.yaml'

kostjuk = KostjukParser()
kostjuk.load(KostjukCatalogue)
csn = CSNParser()
csn.load(CSNCatalogue)
hipparcos = HipParser()
hipparcos.load(HIPCatalogue)
gliese = GlieseParser()
gliese.load(GlieseCatalogue)
ybs = YBSParser()
ybs.load(YBSCatalogue)
soederhjelm = SoederhejlmParser()
soederhjelm.load(SoederhjelmCatalogue)
sb9 = SB9Parser()
sb9.load(SB9Catalogue)
orb6 = ORB6Parser()
orb6.load(ORB6Catalogue)
wds = WdsParser()
wds.load(WDSCatalogue)

with open(StarsFile, 'w') as output_file:
    done = {}
    done_gl = {}
    for star in ybs.stars + gliese.stars:
        coin = False
        binary = False
        gliese_entry = None
        entries = [star]
        hd = star.get('hd', '')
        hr = star.get('hr', '')
        if hd != '' and hd in done:
            continue
        if star['src'] == 'gliese' and star['gl'] in done_gl:
            continue
        if star['src'] == 'ybs' and hd != '':
            gliese_entry = gliese.catalogues['HD'].get(hd)
            if gliese_entry is not None:
                entries.append(gliese_entry)
        hip = ''
        system = None
        if hd is not None:
            hip_entry = hipparcos.catalogues['HD'].get(hd, None)
        if hip_entry is not None:
            hip = hip_entry['hip']
            entries.insert(0, hip_entry)
        xref = None
        if hr != '':
            xref = kostjuk.catalogues['HR'].get(hr, None)
        if xref is None and hd != '':
            xref = kostjuk.catalogues['HD'].get(hd, None)
        if xref is not None:
            if hip == '':
                hip = xref['hip']
        else:
            xref = star
        if hip != '':
            if not isinstance(hip, list):
                hip = [hip]
            for i in hip:
                if i in soederhjelm.astrometric_catalogues['HIP'] or i in soederhjelm.non_astrometric_catalogues['HIP'] or i in has_multiplicity:
                    binary = True
                    if star['src'] == 'gliese':
                        done_gl[star['gl']] = True
                    if gliese_entry is not None:
                        done_gl[gliese_entry['gl']] = True
                    break
        if not binary:
            output_file.write(add_star(csn.names, xref, entries))
        if hd is not None:
            done[hd] = True
    for orbit in soederhjelm.astrometric + soederhjelm.non_astrometric:
        hip = orbit['hip']
        xref = kostjuk.catalogues['HIP'].get(hip)
        hd = None
        if xref is not None:
            hd = xref.get('hd', '')
        stars = []
        if hd != '':
            star = ybs.catalogues['HD'].get(hd, None)
            if star is not None:
                stars.append(star)
            star = gliese.catalogues['HD'].get(hd, None)
            if star is not None:
                stars.append(star)
        star = wds.catalogues['HIP'].get(hip, None)
        if star is not None:
            stars.append(star)
        star = sb9.catalogues['HIP'].get(hip, None)
        if star is not None:
            if xref is None:
                xref = star
            stars.append(star)
        star = orb6.catalogues['HIP'].get(hip, None)
        if star is not None:
            if xref is None:
                xref = star
            stars.append(star)
        system = None
        if system is None:
            system = hipparcos.catalogues['HIP'].get(hip, None)
        if stars is not None:
            if system is not None:
                output_file.write(add_binary_system(csn.names, xref, system, stars, orbit))
            else:
                print("SKIP System", hip)
        else:
            print("SKIP Star", hip)
