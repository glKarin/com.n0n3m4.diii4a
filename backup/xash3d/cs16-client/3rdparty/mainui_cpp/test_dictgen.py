#!/usr/bin/env python3
# test_dictgen.py -- VGUI2 dictionary generator test
# Copyright(C) 2019 a1batross
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import unittest
import dictgen
import filecmp
import os

VALID_UNPARSED_TOKENS = ['L ("test1")', 'L( "test2" )', 'L("test" "test")']
INVALID_UNPARSED_TOKENS = ['L( test )', 'L( "multiline"\n"multiline )']
VALID_TOKENS = ["test1", "test2", "testtest"]

class DictGenTest(unittest.TestCase):

	def test_parsing_srcs(self):
		tokens = dictgen.process_file(os.path.join('tests', 'testfile'))
		for i in VALID_UNPARSED_TOKENS:
			self.assertTrue(i in tokens)

		for i in INVALID_UNPARSED_TOKENS:
			self.assertFalse(i in tokens)

	def test_processing_translables(self):
		tokens = dictgen.process_trans(VALID_UNPARSED_TOKENS)
		self.assertEqual(tokens, VALID_TOKENS)

	def test_vgui_parser(self):
		tokens = dictgen.vgui_translation_parse(os.path.join('tests', 'test_english.txt'))
		self.assertEqual(tokens, VALID_TOKENS)

	def test_dictgen(self):
		dictgen.create_translations_file('temp.txt', VALID_TOKENS)
		tokens = dictgen.vgui_translation_parse('temp.txt')
		os.remove('temp.txt')
		self.assertEqual(tokens, VALID_TOKENS)

if __name__ == '__main__':
	unittest.main()


