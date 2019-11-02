# -*- coding: utf-8 -*-
#
#This file is part of Cosmonium.
#
#Copyright (C) 2018-2019 Laurent Deru.
#
#Cosmonium is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#Cosmonium is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Cosmonium.  If not, see <https://www.gnu.org/licenses/>.
#

from __future__ import absolute_import
from __future__ import print_function

from math import sqrt, pi, modf
import os
import sys
import re
import gzip
import json

G = 6.67430e-11
M0 = 1.98847e30
MU = G * M0

S_YEAR = 365.25 * 60 * 60 * 24

J2000 = 2451545.0
AU = 149597870.700

J1950Jan1_5 = 2433282.5
J1980Jan1_5 = 2444239.5
J1986Jan19 = 2446450.0
J1997Jan16_5 = 2450464.5
J2003Jun10_5 = 2452800.5
J2004Aug25 = 2453243.0

epochs = {
          '1950 jan. 1.00 tt': J1950Jan1_5,
          '1980 jan. 1.0 tt': J1980Jan1_5,
          '1986 jan. 19.50 tt': J1986Jan19,
          '1997 jan. 16.00 tt': J1997Jan16_5,
          '2000 jan. 1.00 tt': J2000 - 0.5,
          '2000 jan. 1.50 tt': J2000,
          '2000 jan. 1.50 tdt': J2000, #TODO: TDT is not TT !
          '2003 jun. 10.00 tt': J2003Jun10_5,
          '2004 aug. 25.50 tt': J2004Aug25
          }

whitespace_regex = re.compile(' +')
perihelion_regex = re.compile('^(\d\d\d\d)(\d\d)(\d\d)(\.\d+)$')
reference_regex = re.compile('\[\d+\]')

def ipart(x):
    return modf(x)[1]

def gcal2jd(year, month, day):
    year = int(year)
    month = int(month)
    day = int(day)

    a = ipart((month - 14) / 12.0)
    jd = ipart((1461 * (year + 4800 + a)) / 4.0)
    jd += ipart((367 * (month - 2 - 12 * a)) / 12.0)
    x = ipart((year + 4900 + a) / 100.0)
    jd -= ipart((3 * x) / 4.0)
    jd += day - 2432075.5  # was 32075; add 2400000.5

    jd -= 0.5  # 0 hours; above JD is for midday, switch to midnight.

    return jd

def dump_frame(stream, frame, ra, dec):
    if frame == 'J2000Ecliptic':
        stream.write("    frame: J2000Ecliptic\n")
    elif frame is 'J2000Equatorial':
        stream.write("    frame: J2000Equatorial\n")
    elif frame.startswith('iau:'):
        stream.write("    frame: %s\n" % frame)
    else:
        stream.write("    frame:\n")
        stream.write("        type: equatorial\n")
        stream.write("        ra: %s\n" % ra)
        stream.write("        de: %s\n" % dec)

def dump_orbit(stream, name, p, a, e, w, m, i, o, epoch, frame, ra, dec, distance='km', period='day'):
    stream.write("- orbit:\n")
    stream.write("    name: %s\n" % name)
    stream.write("    type: elliptic\n")
    stream.write("    category: ssd\n")
    stream.write("    period: %s\n" % p)
    stream.write("    period-units: %s\n" % period)
    stream.write("    semi-major-axis: %s\n" % a)
    stream.write("    semi-major-axis-units: %s\n" % distance)
    stream.write("    eccentricity: %s\n" % e)
    stream.write("    inclination: %s\n" % i)
    stream.write("    arg-of-periapsis: %s\n" % w)
    stream.write("    ascending-node: %s\n" % o)
    stream.write("    mean-anomaly: %s\n" % m)
    if epoch != J2000:
        stream.write("    epoch: %f\n" % epoch)
    dump_frame(stream, frame, ra, dec)

def dump_orbit_2(stream, name, p, a, e, i, l, w, o, epoch, frame="J2000Ecliptic", ra=None, dec=None, distance='AU', period='year'):
    stream.write("- orbit:\n")
    stream.write("    name: %s\n" % name)
    stream.write("    type: elliptic\n")
    stream.write("    category: ssd\n")
    stream.write("    period: %s\n" % p)
    stream.write("    period-units: %s\n" % period)
    stream.write("    semi-major-axis: %s\n" % a)
    stream.write("    semi-major-axis-units: %s\n" % distance)
    stream.write("    eccentricity: %s\n" % e)
    stream.write("    inclination: %s\n" % i)
    stream.write("    long-of-pericenter: %s\n" % w)
    stream.write("    ascending-node: %s\n" % o)
    stream.write("    mean-longitude: %s\n" % l)
    if epoch != J2000:
        stream.write("    epoch: %f\n" % epoch)
    dump_frame(stream, frame, ra, dec)

def dump_sat(stream, name, radius, orbit, rotation, albedo):
    stream.write("- minormoon:\n")
    stream.write("    name: %s\n" % name)
    stream.write("    radius: %s\n" % radius)
    if albedo is not None:
        stream.write("    albedo: %s\n" % albedo)
    stream.write("    surfaces:\n")
    stream.write("      - category: uniform\n")
    stream.write("        heightmap: asteroid\n")
    stream.write("        appearance:\n")
    stream.write("            diffuse-color: [0.3, 0.3, 0.3, 1]\n")
    stream.write("        lighting-model: oren-nayar\n")
    stream.write("    orbit: %s\n" % orbit)
    stream.write("    rotation: %s\n" % rotation)

def parse_ssd_data(stream, data, parent_name):
    laplacian = False
    ecliptic = False
    epoch = J2000
    for line in data:
        line = line.lower()
        if line == '': continue
        if line.startswith('solution'):
            continue
        elif "mean" in line:
            print("Found new block", line)
            ecliptic = False
            laplacian = False
            if 'laplacian' in line or 'laplace' in line:
                laplacian = True
            elif 'ecliptic' in line:
                ecliptic = True
        elif "epoch" in line:
            epoch = 0
            for (epoch_text, value) in epochs.items():
                if epoch_text in line:
                    epoch = value
                    print("Found new epoch", line)
                    break
            if epoch == 0:
                print("Unknown epoch", line)
                epoch = J2000
        elif "sat." in line:
            print("found new header")
        elif "(" in line:
            #skip units
            pass
        else:
            ra = None
            dec = None
            elements = line.split()
            if '/' in elements[0]:
                name = elements.pop(0) + elements.pop(0) + elements.pop(0)
                elements.insert(0, name)
            if laplacian:
                (name, a, e, w, m, i, node, n, p ,pw, pnode, ra, dec, tilt, ref) = elements
                frame = 'Laplacian'
            else:
                (name, a, e, w, m, i, node, n, p ,pw, pnode, ref) = elements
                if ecliptic:
                    frame = 'J2000Ecliptic'
                else:
                    frame = 'iau:%s' % parent_name
            dump_orbit(stream, name, p, a, e, w, m, i, node, epoch, frame, ra, dec)

def parse_ssd_sat_file(stream, filepath, name):
    with open(filepath, 'r') as data:
        parse_ssd_data(stream, map(lambda x: x.rstrip('\r\n'), data), name)

def dump_ssd_sat_file(stream, filepath):
    basedir = os.path.dirname(filepath)
    with open(filepath, 'r') as source:
            for line in source.readlines():
                line = line.strip()
                if line == '': continue
                (name, filename, phy_filename) = line.split(' ')
                parse_ssd_sat_file(stream, os.path.join(basedir, filename), name)

def parse_ssd_sat_phy_file(filepath):
    db = []
    with open(filepath, 'r') as data:
        for line in data.readlines():
            line = reference_regex.sub('', line)
            line = line.strip()
            if line == '': continue
            elements = whitespace_regex.split(line)
            if len(elements) > 6:
                name = elements.pop(0)
                elements[0] = name + ' ' + elements[0]
            db.append(elements)
    return db

def dump_ssd_sat_phy_file(stream, db, skip_dp):
    for entry in db:
        (name, gm, radius, density, magnitude, albedo) = entry
        if name in skip_dp: continue
        orbit = name.lower().replace(' ', '')
        rotation = 'unknown'
        radius = radius.split('±')[0]
        albedo = albedo.split('±')[0]
        if albedo == '?': albedo = None
        dump_sat(stream, name, radius, orbit, rotation, albedo)
        
def dump_ssd_sat_phy_files(filepath, skip_phy):
    basedir = os.path.dirname(filepath)
    skip_db = []
    with open(skip_phy, 'r') as skip:
        for line in skip.readlines():
            line = line.strip()
            if line == '': continue
            skip_db.append(line)
    with open(filepath, 'r') as source:
        for line in source.readlines():
            line = line.strip()
            if line == '': continue
            (name, filename, phy_filename) = line.split(' ')
            if phy_filename == 'x': continue
            db = parse_ssd_sat_phy_file(os.path.join(basedir, phy_filename))
            with open(os.path.join('solar-system', name, 'irregular-moons.yaml'), 'w') as stream:
                dump_ssd_sat_phy_file(stream, db, skip_db)

def parse_sbd_line(line):
    (text_id, name) = whitespace_regex.split(line[0:25].strip(), maxsplit=1)
    (epoch, a, e, i, w, node, m, h, g, ref) = whitespace_regex.split(line[25:].strip(), maxsplit=9)
    return (text_id, (text_id, name, epoch, a, e, i, w, node, m, h, g, ref))

def parse_sbd_comet_line(line):
    (text_id, name) = line[0:44].strip().split('/', 1)
    (epoch, q, e, i, w, node, tp, ref) = whitespace_regex.split(line[44:].strip(), maxsplit=7)
    return (text_id, (text_id, name, epoch, q, e, i, w, node, tp, ref))

def parse_sbd_db(filepath, comet=False):
    count = 0
    db = {}
    if filepath.endswith('.gz'):
        with gzip.open(filepath, 'r') as data:
            for line in data.readlines():
                count += 1
                if count <= 2: continue
                if comet:
                    (text_id, elements) = parse_sbd_comet_line(line)
                else:
                    (text_id, elements) = parse_sbd_line(line)
                db[text_id] = elements
    else:
        with open(filepath, 'r') as data:
            for line in data.readlines():
                count += 1
                if count <= 2: continue
                if comet:
                    (text_id, elements) = parse_sbd_comet_line(line)
                else:
                    (text_id, elements) = parse_sbd_line(line)
                db[text_id] = elements
    return db

def parse_sbd_file(filepath):
    result = []
    with open(filepath, 'r') as data:
        for line in data.readlines():
            line = line.strip()
            if line == '': continue
            entries = whitespace_regex.split(line, maxsplit=1)
            text_id = entries[0]
            result.append(text_id)
    return result

def dump_sbd_asteroid(stream, elements):
        (text_id, name, epoch, a, e, i, w, node, m, h, g, ref) = elements
        fa = float(a) * AU * 1000
        p = 2 * pi * sqrt(fa*fa*fa / MU) / S_YEAR
        if name == 'Pluto':
            name = 'pluto-system'
        elif name == 'Varda':
            name = 'varda-system'
        elif name == 'Metis':
            name = '9-metis'
        else:
            name = name.lower().replace(' ', '')
        dump_orbit(stream, name, p, a, e, w, m, i, node, float(epoch) + 2400000.5, 'J2000Ecliptic', None, None, distance='AU', period='year')

def dump_sbd_comet(stream, elements):
    (text_id, name, epoch, q, e, i, w, node, tp, ref) = elements
    a = float(q) / (1.0 - float(e))
    fa = a * AU * 1000
    p = 2 * pi * sqrt(fa*fa*fa / MU)
    epoch = float(epoch) + 2400000.5
    m = perihelion_regex.match(tp)
    (year, month, day, frac) = m.groups()
    tp = gcal2jd(int(year), int(month), int(day)) + float(frac) + 2400000.5
    n = 2 * pi / p * 86400
    m = n * (epoch - float(tp)) * 180 / pi
    dump_orbit(stream, text_id.lower(), p / S_YEAR, a, e, w, m, i, node, epoch, 'J2000Ecliptic', None, None, distance='AU', period='year')

def dump_sbd(stream, db, db_comet, entries):
    for text_id in entries:
        elements = db.get(text_id)
        if elements is not None:
            dump_sbd_asteroid(stream, elements)
            continue
        elements = db_comet.get(text_id)
        if elements is not None:
            dump_sbd_comet(stream, elements)
            continue
        print("Entry '%s' not found" % text_id)

def parse_mpcorb_db(filepath):
    if filepath.endswith('.gz'):
        with gzip.open(filepath, 'r') as data:
            db = json.load(data)
    else:
        with open(filepath, 'r') as data:
            db = json.load(data)
    db_dict = {}
    for entry in db:
        try:
            db_dict[entry['Number']] = entry
        except KeyError:
            pass
    return db_dict

def parse_mpcorb_file(filepath):
    result = []
    with open(filepath, 'r') as data:
        for line in data.readlines():
            line = line.strip()
            if line == '': continue
            entries = whitespace_regex.split(line, maxsplit=1)
            text_id = entries[0]
            result.append(text_id)
    return result

def dump_mpcorb(stream, db, entries):
    for entry in entries:
        try:
            text_id = '(' + entry + ')'
            elements = db[text_id]
            if 'Name' in elements:
                name = elements['Name'].lower()
            else:
                name = elements['Principal_desig'].lower().replace(' ', '')
            dump_orbit(stream,
                       name,
                       elements['Orbital_period'],
                       elements['a'],
                       elements['e'],
                       elements['Peri'],
                       elements['M'],
                       elements['i'],
                       elements['Node'],
                       elements['Epoch'],
                       'J2000Ecliptic',
                       None, None,
                       distance='AU',
                       period='year')
        except KeyError:
            print("Entry '%s' not found" % entry)

def parse_planet_1(filepath):
    db = {}
    header = True
    with open(filepath, 'r') as data:
        for line in data.readlines():
            if line.startswith('   '): continue
            line = line.strip()
            if line == '': continue
            if header:
                if line.startswith('---------'):
                    header = False
                continue
            (name, a, e, i, l, w, node) = whitespace_regex.split(line, maxsplit=6)
            elements = (name, a, e, i, l, w, node)
            db[name] = elements
    return db

def parse_planet_phy(filepath):
    db = {}
    with open(filepath, 'r') as data:
        for line in data.readlines():
            line = line.strip()
            if line == '': continue
            (name, equatorial, mean, mass, density, rotation, orbit, v, albedo, gravity, velocity) = whitespace_regex.split(line, maxsplit=10)
            elements = (name, equatorial, mean, mass, density, rotation, orbit, v, albedo, gravity, velocity)
            db[name.lower()] = elements
    return db

def dump_planet_db(stream, db, phy_db):
    for entry in db.values():
        (name, a, e, i, l, w, node) = entry
        if name == 'EM_Bary':
            name = 'Earth'
        (name, equatorial, mean, mass, density, rotation, orbit, v, albedo, gravity, velocity) = phy_db[name.lower()]
        if name == 'Earth':
            name = 'earth-system'
        elif name == 'Pluto':
            name = 'pluto-system'
        dump_orbit_2(stream, name.lower(), orbit, a, e, i, l, w, node, J2000)

def dump_info(stream):
    stream.write("# This file is automatically generated using the orbital elements found at SSD\n")
    stream.write("# https://ssd.jpl.nasa.gov/ (C) NASA/JPL-Caltech\n")
    stream.write("#\n")
    stream.write("# Do not modify, if an orbit needs adjustment, use manual-orbits.yaml instead.\n")

def dump_category(stream):
    stream.write("- orbit-category:\n")
    stream.write("    name: ssd\n")
    stream.write("    priority: 1\n")

sat_file = 'data/ssd/ssd.txt'
sbd_db = 'data/ssd/ELEMENTS.NUMBR.gz'
sbd_comet_db = 'data/ssd/ELEMENTS.COMET'
planets_db = 'data/ssd/planets.txt'
planets_phy = 'data/ssd/planets-phy.txt'
skip_phy = 'data/ssd/skip-phy.txt'
sbdb_file = 'data/ssd/sbdb.txt'
out_file = 'solar-system/ssd.yaml'

def main():
    #(sat_file, mpcorb_db, mpcorb_file, out_file) = sys.argv[1:]
    #db = parse_mpcorb_db(mpcorb_db)
    #entries = parse_mpcorb_file(mpcorb_file)
    dump_ssd_sat_phy_files(sat_file, skip_phy)
    planet_db = parse_planet_1(planets_db)
    planet_phy = parse_planet_phy(planets_phy)
    db = parse_sbd_db(sbd_db)
    db_comet = parse_sbd_db(sbd_comet_db, comet=True)
    entries = parse_sbd_file(sbdb_file)

    with open(out_file, 'w') as stream:
        dump_info(stream)
        dump_category(stream)
        dump_planet_db(stream, planet_db, planet_phy)
        dump_ssd_sat_file(stream, sat_file)
        #dump_mpcorb(stream, db, entries)
        dump_sbd(stream, db, db_comet, entries)

main()
