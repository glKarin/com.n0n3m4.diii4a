#!/usr/bin/python3
#  Copyright (c) 2016-2016 David Anderson.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the example nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS'' AND ANY
#  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL David Anderson BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
#  OF SUCH DAMAGE.


import sys

# Use only <pre> or </pre> all by itself in data.xml.
# No other data on either of such lines.
# All the lines between these two markers should be
# shown in individual lines.
def xmlize(linea, inhtml, inpre):
    outi = []
    l = linea
    if l.find("<pre>") != -1:
        if inhtml == "y":
            s2 = "</p>" + l + "\n"
        else:
            s2 = l + "\n"
        inpre = "y"
        return s2, inpre
    if l.find("</pre>") != -1:
        if inhtml == "y":
            s2 = l + "\n" + "<p>"
        else:
            s2 = l + "\n"
        inpre = "n"
        return s2, inpre
    if inpre == "y" and inhtml == "n":
        outi += ["<preline>"]
    for c in l:
        if c == "<":
            outi += ["&lt;"]
        elif c == ">":
            outi += ["&gt;"]
        elif c == "&":
            outi += ["&amp;"]
        # elif c == "'":
        #   outi += ["&apos;"]
        elif c == '"':
            outi += ["&quot;"]
        else:
            outi += [c]
    if inpre == "y" and inhtml == "n":
        outi += ["</preline>"]
    outi += ["\n"]
    s2 = "".join(outi)
    return s2, inpre


def paraline(name, linea):
    out = ""
    strp = linea.strip()
    if len(strp) < 1:
        out = "<p>" + name + ":" + "</p>"
        return out
    out = "<p>" + name + ": "
    out += linea
    out += "</p>"
    return out


def paralines(name, lines):
    inpre = "n"
    if len(lines) < 1:
        out = "<p>" + name + ":" + "</p>"
        return out
    out = "<p>" + name + ":"
    for lin in lines:
        f, inpre = xmlize(lin, "y", inpre)
        out += f
    out += "</p>"
    return out


def para(name, str):
    if str == None:
        out = "<p>" + name + ":" + "</p>"
    elif len(str) > 0:
        out = "<p>" + name + ":&nbsp" + str + "</p>"
    else:
        out = "<p>" + name + ":" + "</p>"
    return out


class bugrecord:
    def __init__(self, dwid):
        self._id = dwid.strip()
        self._cve = ""
        self._fuzzer = ""
        self._datereported = ""
        self._reportedby = ""
        self._vulnerability = []
        self._product = ""
        self._description = []
        self._datefixed = ""
        self._references = []
        self._gitfixid = ""
        self._tarrelease = ""

    def setcve(self, pubid):
        if self._cve != "":
            print("Duplicate cve ", self._cve, pubid)
            sys.exit(1)
        self._cve = pubid.strip()
    def setfuzzer(self, id):
        if self._fuzzer != "":
            print("Duplicate fuzzer ", self._fuzzer, id)
            sys.exit(1)
        self._fuzzer = id.strip()
    def setdatereported(self, rep):
        if self._datereported != "":
            print("Duplicate datereported ", self._datereported, rep)
            sys.exit(1)
        self._datereported = rep.strip()

    def setreportedby(self, rep):
        if self._reportedby != "":
            print("Duplicate reportedby ", self._reportedby, rep)
            sys.exit(1)
        self._reportedby = rep.strip()

    def setvulnerability(self, vuln):
        if len(self._vulnerability) != 0:
            print("Duplicate vulnerability ", self._vulnerability, vuln)
            sys.exit(1)
        self._vulnerability = vuln

    def setproduct(self, p):
        if len(self._product) != 0:
            print("Duplicate product ", self._product, p)
            sys.exit(1)
        self._product = p.strip()

    def setdescription(self, d):
        if len(self._description) != 0:
            print("Duplicate description ", self._description, d)
            sys.exit(1)
        self._description = d

    def setdatefixed(self, d):
        if len(self._datefixed) != 0:
            print("Duplicate datefixed ", self._datefixed, d)
            sys.exit(1)
        self._datefixed = d.strip()

    def setreferences(self, r):
        if len(self._references) != 0:
            print("Duplicate references ", self._references, r)
            sys.exit(1)
        self._references = r

    def setgitfixid(self, g):
        if len(self._gitfixid) != 0:
            print("Duplicate gitfixid ", self._gitfixid, g)
            sys.exit(1)
        self._gitfixid = g.strip()

    def settarrelease(self, g):
        if len(self._tarrelease) != 0:
            print("Duplicate tarrelease ", self._tarrelease, g)
            sys.exit(1)
        self._tarrelease = g.strip()

    def plist(self, title, lines):
        if lines == None:
            print(title)
            return
        if len(lines) == 1:
            print(title, lines[0])
            return
        print(title)
        for l in lines:
            print(l)

    def printbug(self):
        print("")
        print("id:", self._id)
        print("cve:", self._cve)
        print("fuzzer:", self._fuzzer)
        print("datereported:", self._datereported)
        print("reportedby:", self._reportedby)
        self.plist("vulnerability:", self._vulnerability)
        print("product:", self._product)
        self.plist("description:", self._description)
        print("datefixed:", self._datefixed)
        self.plist("references:", self._references)
        print("gitfixid:", self._gitfixid)
        print("tarrelease:", self._tarrelease)

    def generate_html(self,num):
        s5 = "".join(self._id)
        idhead = ''.join([str(num),") ",self._id])
        t = "".join(['<h3 id="',s5, '">',idhead, "</h3>"])
        txt = [t]

        inpre = "n"
        s, inp = xmlize(self._id, "y", inpre)
        t = paraline("id", s)
        txt += [t]

        s, inp = xmlize(self._cve, "y", inpre)
        t = paraline("cve", s)
        txt += [t]

        s, inp = xmlize(self._fuzzer, "y", inpre)
        t = paraline("fuzzer", s)
        txt += [t]

        s, inp = xmlize(self._datereported, "y", inpre)
        t = paraline("datereported", s)
        txt += [t]

        s, inp = xmlize(self._reportedby, "y", inpre)
        t = paraline("reportedby", s)
        txt += [t]

        # MULTI
        t = paralines("vulnerability", self._vulnerability)
        txt += [t]

        s, inp = xmlize(self._product, "y", inpre)
        t = paraline("product", s)
        txt += [t]

        # MULTI
        t = paralines("description", self._description)
        txt += [t]

        s, inp = xmlize(self._datefixed, "y", inpre)
        t = paraline("datefixed", s)
        txt += [t]

        # MULTI
        t = paralines("references", self._references)
        txt += [t]

        s, inp = xmlize(self._gitfixid, "y", inpre)
        t = paraline("gitfixid", s)
        txt += [t]

        s, inp = xmlize(self._tarrelease, "y", inpre)
        t = paraline("tarrelease", s)
        txt += [t]

        t = '<p> <a href="#top">[top]</a> </p>'
        txt += [t]
        return txt

    def paraxml(self, start, main, term):
        # For single line xml remove the newline from the main text line.
        out = start
        l = main.strip()
        if len(l) > 0:
            out += l
        out += term 
        return out

    def paraxmlN(self, start, main, term):
        # For multi line xml leave newlines present.
        out = start
        inpre = "n"
        for x in main:
            l = x.rstrip()
            t, inpre = xmlize(l, "n", inpre)
            if len(t) > 0:
                out += t
        out += term 
        return out

    def generate_xml(self):
        txt = []
        t = "<dwbug>"
        txt += [t]

        inpre = "n"
        s, inpre = xmlize(self._id, "n", inpre)
        t = self.paraxml("<dwid>", s, "</dwid>")
        txt += [t]

        s, inpre = xmlize(self._cve, "n", inpre)
        t = self.paraxml("<cve>", s, "</cve>")
        txt += [t]

        s, inpre = xmlize(self._fuzzer, "n", inpre)
        t = self.paraxml("<fuzzer>", s, "</fuzzer>")
        txt += [t]

        s, inpre = xmlize(self._datereported, "n", inpre)
        t = self.paraxml("<datereported>", s, "</datereported>")
        txt += [t]

        s, inpre = xmlize(self._reportedby, "n", inpre)
        t = self.paraxml("<reportedby>", s, "</reportedby>")
        txt += [t]

        s, inpre = xmlize(self._product, "n", inpre)
        t = self.paraxml("<product>", s, "</product>")
        txt += [t]

        # MULTI
        p = self._vulnerability
        t = self.paraxmlN("<vulnerability>", p, "</vulnerability>")
        txt += [t]

        # MULTI
        p = self._description
        t = self.paraxmlN("<description>", p, "</description>")
        txt += [t]

        s, inpre = xmlize(self._datefixed, "n", inpre)
        t = self.paraxml("<datefixed>", s, "</datefixed>")
        txt += [t]

        # MULTI
        p = self._references
        t = self.paraxmlN("<references>", p, "</references>")
        txt += [t]

        s, inpre = xmlize(self._gitfixid, "n", inpre)
        t = self.paraxml("<gitfixid>", s, "</gitfixid>")
        txt += [t]

        s, inpre = xmlize(self._tarrelease, "n", inpre)
        t = self.paraxml("<tarrelease>", s, "</tarrelease>")
        txt += [t]

        t = "</dwbug>"
        txt += [t]
        return txt
