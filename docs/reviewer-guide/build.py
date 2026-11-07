#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Render the source-pinned author-review guide; not a runtime build dependency."""
from pathlib import Path
import hashlib, html, json, os, re, textwrap
from reportlab import Version
from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.utils import simpleSplit
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph,
    Spacer, PageBreak, CondPageBreak, Table, TableStyle, Flowable, Preformatted,
    KeepTogether)
from reportlab.platypus.tableofcontents import TableOfContents

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
OUT = REPO / 'output/pdf/bsd-cubed-reviewer-guide.pdf'
INK = colors.HexColor('#173247'); TEAL = colors.HexColor('#087F8C')
MUTED = colors.HexColor('#536370'); PAPER = colors.HexColor('#F3F6F7')
RULE = colors.HexColor('#D1DDE2'); WHITE = colors.white
W, H = 612, 792
LEFT = 72; WIDTH = 468
CODE_REF = '8bfa868f3a1c485954d35b9be28b2347b2e4bf30'
DATE = '5 November 2026'

runtime = Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/native/libreoffice-headless/libreoffice/LibreOfficeDev.app/Contents/Resources/fonts/truetype'
candidates = [Path(os.environ.get('BSD3_DOC_FONT_DIR', str(runtime))), Path('/usr/share/fonts/truetype/dejavu')]
fontdir = next((p for p in candidates if (p/'DejaVuSans.ttf').exists()), None)
if fontdir is None: raise SystemExit('Set BSD3_DOC_FONT_DIR to a directory containing the documented fonts')
serif = 'NotoSerif' if (fontdir/'NotoSerif-Regular.ttf').exists() else 'DejaVuSerif'
fontfiles = {
 'Serif': 'NotoSerif-Regular.ttf' if serif=='NotoSerif' else 'DejaVuSerif.ttf',
 'SerifB': f'{serif}-Bold.ttf',
 'SerifI': f'{serif}-Italic.ttf',
 'Sans': 'DejaVuSans.ttf', 'SansB':'DejaVuSans-Bold.ttf',
 'Mono':'DejaVuSansMono.ttf',
}
for name,file in fontfiles.items(): pdfmetrics.registerFont(TTFont(name,str(fontdir/file)))
pdfmetrics.registerFontFamily('Serif',normal='Serif',bold='SerifB',italic='SerifI',boldItalic='SerifB')
pdfmetrics.registerFontFamily('Sans',normal='Sans',bold='SansB',italic='Sans',boldItalic='SansB')
styles = {
 'body':ParagraphStyle('Body',fontName='Serif',fontSize=10.2,leading=14.5,textColor=INK,
     spaceAfter=8,allowWidows=0,allowOrphans=0,splitLongWords=1),
 'h1':ParagraphStyle('H1',fontName='SansB',fontSize=16.2,leading=21,textColor=INK,
     spaceBefore=17,spaceAfter=10,keepWithNext=True),
 'h2':ParagraphStyle('H2',fontName='SansB',fontSize=12,leading=16,textColor=TEAL,
     spaceBefore=12,spaceAfter=7,keepWithNext=True),
 'caption':ParagraphStyle('Caption',fontName='Sans',fontSize=8.3,leading=11.3,textColor=MUTED,
     spaceBefore=5,spaceAfter=12),
 'cell':ParagraphStyle('Cell',fontName='Serif',fontSize=8.7,leading=11.8,textColor=INK,
     spaceAfter=0,splitLongWords=1),
 'th':ParagraphStyle('TH',fontName='SansB',fontSize=8.5,leading=11.5,textColor=WHITE),
 'ref':ParagraphStyle('Reference',fontName='Serif',fontSize=8.0,leading=10.5,textColor=INK,
     spaceAfter=5,splitLongWords=1),
 'box':ParagraphStyle('Box',fontName='Sans',fontSize=9.6,leading=14,textColor=INK,spaceAfter=0),
 'code':ParagraphStyle('Code',fontName='Mono',fontSize=7.4,leading=11,textColor=INK,
     backColor=PAPER,borderPadding=10,spaceBefore=8,spaceAfter=12),
}

def inline(s, small=False):
    parts = re.split(r'(`[^`]+`|https?://[^\s]+)',s)
    out=[]
    for p in parts:
        if p.startswith('`') and p.endswith('`'):
            out.append(f'<font name="Mono" size="{7.6 if small else 8.4}">{html.escape(p[1:-1])}</font>')
        elif p.startswith(('http://','https://')):
            suffix=''
            while p.endswith(('.',',',';')):suffix=p[-1]+suffix;p=p[:-1]
            out.append(f'<a href="{html.escape(p,quote=True)}" color="#087F8C">{html.escape(p)}</a>{suffix}')
        else:
            p=html.escape(p)
            p=re.sub(r'\*\*([^*]+)\*\*',r'<b>\1</b>',p)
            out.append(p)
    return ''.join(out)

class Cover(Flowable):
    def __init__(self):
        super().__init__(); self.width=WIDTH; self.height=650
    def draw(self):
        c=self.canv
        c.setFillColor(TEAL);c.setFont('SansB',8.7)
        c.drawString(0,618,'SYSTEMS ENGINEERING / REVIEWER EDITION')
        c.setFillColor(INK);c.setFont('SerifB',56);c.drawString(0,531,'BSD³')
        c.setFont('SerifB',28);c.drawString(0,479,'A native desktop bridge')
        c.drawString(0,443,'for FreeBSD')
        c.setFillColor(MUTED);c.setFont('Sans',12)
        c.drawString(0,402,'Architecture, implementation and experimental validation')
        c.setFillColor(INK);c.setFont('Serif',17);c.drawString(0,339,'Denis Lemire')
        c.setFont('Sans',10.2)
        c.drawString(0,315,'Aspiring independent contributor and FreeBSD enthusiast')
        c.setFillColor(PAPER);c.rect(0,203,WIDTH,67,fill=1,stroke=0)
        c.setFillColor(TEAL);c.rect(0,203,3,67,fill=1,stroke=0)
        c.setFont('SansB',8.4);c.setFillColor(INK)
        c.drawString(14,249,'AUTHOR REVIEW DRAFT - HARDWARE VALIDATION PENDING')
        c.setFont('Serif',10)
        c.drawString(14,229,'A little daemon with no ambition to become PID 1.')
        c.setStrokeColor(TEAL);c.setLineWidth(1.3);c.line(0,159,WIDTH,159)
        for x,title,desc in [(0,'REGISTER','Verified ordinary-user sessions'),(160,'CONTROL','Original identity and device leases'),(320,'RECOVER','Bounded, observed console restore')]:
            c.setFillColor(INK);c.setFont('SansB',9);c.drawString(x,140,title)
            c.setFillColor(MUTED);c.setFont('Sans',7.3);c.drawString(x,121,desc)
        c.setFont('Sans',8.2);c.drawString(0,67,f'AUTHOR REVIEW DRAFT / v0.1 / {DATE}')
        c.setFont('Mono',7);c.drawString(0,47,f'Implementation reference: {CODE_REF}')
        c.setFont('Serif',8.5);c.drawString(0,23,'Not a FreeBSD Project publication or an assertion of upstream acceptance.')

class Diagram(Flowable):
    def __init__(self,kind): super().__init__();self.kind=kind;self.width=WIDTH;self.height={'architecture':310,'leases':245,'startup':295,'recovery':235}[kind]
    def box(self,x,y,w,h,lines,title=True,fill=PAPER):
        c=self.canv;c.setFillColor(fill);c.setStrokeColor(TEAL);c.setLineWidth(.65)
        c.roundRect(x,y,w,h,3,fill=1,stroke=1)
        wrapped=[]
        for i,line in enumerate(lines):
            font='SansB' if i==0 and title else 'Sans'
            size=9 if i==0 and title else 8
            for row in simpleSplit(line,font,size,w-16):wrapped.append((row,font,size))
        yy=y+h/2+(len(wrapped)-1)*5.7
        for row,font,size in wrapped:
            c.setFillColor(INK if font=='SansB' else MUTED);c.setFont(font,size)
            c.drawCentredString(x+w/2,yy-2,row);yy-=11.4
    def arrow(self,x1,y1,x2,y2):
        import math
        c=self.canv;c.setStrokeColor(TEAL);c.setFillColor(TEAL);c.setLineWidth(.9);c.line(x1,y1,x2,y2)
        ang=math.atan2(y2-y1,x2-x1);a=4
        p=c.beginPath();p.moveTo(x2,y2);p.lineTo(x2-a*math.cos(ang-.45),y2-a*math.sin(ang-.45));p.lineTo(x2-a*math.cos(ang+.45),y2-a*math.sin(ang+.45));p.close();c.drawPath(p,fill=1,stroke=0)
    def label(self,x,y,text,size=7.3):
        self.canv.setFillColor(MUTED);self.canv.setFont('Sans',size);self.canv.drawCentredString(x,y,text)
    def draw(self):
        c=self.canv;k=self.kind
        if k=='architecture':
            for x,w in [(0,215),(235,233)]:c.setFillColor(PAPER);c.roundRect(x,4,w,300,4,fill=1,stroke=0)
            self.label(107,287,'ORDINARY DESKTOP PROCESS',8);self.label(351,287,'PRIVILEGED BACKEND BOUNDARY',8)
            self.box(10,220,185,46,['GNOME / Mutter','GNOME47 baseline'])
            self.box(10,130,185,55,['libbsd3-login.a','Selected ABI / typed CK discovery'])
            self.box(10,29,185,59,['Explicit embedding','PIC archive; ordinary allocation','No fake libsystemd version'])
            self.box(245,220,213,46,['bsddd / login1 subset','Original sender UID/PID/session'])
            self.box(245,120,213,64,['ConsoleKit2','Pinned owner; isolated controller','Session truth and leased FDs'])
            self.box(245,18,213,70,['FreeBSD kernel / devfs','VT and evdev tested in KVM','Physical DRM/input pending'])
            self.arrow(102,220,102,185);self.arrow(195,243,245,243);self.label(220,250,'IPC')
            self.arrow(195,157,245,157);self.label(220,164,'IPC');self.arrow(351,220,351,184);self.arrow(351,120,351,88)
        elif k=='leases':
            self.box(5,151,128,45,['Active lease','Actual descriptor handed'])
            self.box(170,151,128,45,['Awaiting ACK','Normal pause'])
            self.box(335,151,128,45,['Paused','No outstanding ACK'])
            self.box(335,40,128,57,['Fresh active lease','Resume supplies new FD','Old FD stays revoked'])
            self.box(5,40,128,57,['Removed','Release / gone /','controller teardown'])
            self.arrow(133,173,170,173);self.arrow(298,173,335,173);self.label(151,182,'pause');self.label(316,182,'ACK')
            self.arrow(399,151,399,97);self.label(422,123,'resume')
            self.arrow(69,151,69,97);self.label(117,122,'release')
            c.setStrokeColor(TEAL);c.setLineWidth(.9);c.line(69,196,69,220);c.line(69,220,399,220);self.arrow(399,220,399,196);self.label(235,226,'force pause: no acknowledgement required')
            self.label(235,15,'Selected transitions. Invalid ordering and uncertain cleanup drop the controller.',7)
        elif k=='startup':
            self.box(0,250,210,35,['SDDM helper / PAM','Privileged entry'])
            self.box(0,160,210,68,['Ordinary bsd3-session','Real foreground controlling VT','UID / local / type / class / active'])
            self.box(268,160,200,68,['ConsoleKit registration','New owned or validated inherited','Private cookie and runtime path'])
            self.box(0,47,210,67,['Ordinary child','Session bus + upstream profile','GNOME command; stdin retained'])
            self.box(268,47,200,67,['Parent lifetime','Pin backend; supervise descendants','Close only owned registration'])
            self.arrow(105,250,105,228);self.arrow(210,204,268,204);self.label(239,212,'register')
            self.arrow(268,183,210,183);self.label(239,171,'verify')
            self.arrow(105,160,105,114);self.label(161,134,'verified env')
            self.label(234,17,'This ordinary parent is not PID 1 or a replacement user-service graph.',7.2)
        elif k=='recovery':
            for x,y,lines in [(5,150,['Snapshot','Refuse controlled baseline']),(170,150,['Ready','Guard + controller + live output']),(335,150,['Workload','Independent inner deadline']),(335,49,['Exit / cleanup','Status is not final recovery']),(170,49,['Restore / read back','Fresh handles; original VT']),(5,49,['Complete','PASS or failed/assisted'])]:self.box(x,y,128,53,lines)
            self.arrow(133,176,170,176);self.arrow(298,176,335,176);self.arrow(399,150,399,102);self.arrow(335,76,298,76);self.arrow(170,76,133,76)
            self.label(234,20,'The final barrier follows the observation and restoration; never manufacture it.',7.1)

CAPTIONS={
 'architecture':'Figure 1. Two desktop-facing channels with different trust and lifetime obligations. The backend remains a separately licensed privileged component.',
 'leases':'Figure 2. Device-lease transitions. Mock FD transport and actual kernel revocation supply different evidence; both are needed.',
 'startup':'Figure 3. Strict ordinary-user startup preserves the controlling VT and registration lifetime before and during the desktop command.',
 'recovery':'Figure 4. A bounded trial has an explicit final recovery barrier. A clean restore after a timeout remains assisted recovery.',
}

class GuideDoc(BaseDocTemplate):
    def afterFlowable(self,f):
        if isinstance(f,Paragraph) and f.style.name in ('H1','H2'):
            title=f.getPlainText();level=0 if f.style.name=='H1' else 1
            key='section-'+hashlib.sha1(title.encode()).hexdigest()[:12]
            self.canv.bookmarkPage(key);self.canv.addOutlineEntry(title,key,level=level,closed=False)
            self.notify('TOCEntry',(level,title,self.page,key))

def page(c,doc):
    if doc.page==1:return
    c.saveState();c.setFillColor(MUTED);c.setFont('Sans',7.8)
    c.drawString(LEFT,H-39,'BSD³ / NATIVE DESKTOP BRIDGE')
    c.drawRightString(W-LEFT,H-39,'Denis Lemire')
    c.setStrokeColor(RULE);c.setLineWidth(.45);c.line(LEFT,H-47,W-LEFT,H-47)
    c.setFont('Sans',7);c.drawString(LEFT,36,f'Author review draft / {DATE} / source {CODE_REF[:7]}')
    c.setFont('Sans',8.3);c.drawRightString(W-LEFT,36,str(doc.page));c.restoreState()

def table(rows):
    cols=len(rows[0])
    if cols==2:
        widths=[150,318] if 'SHA256' in rows[0][1] else [145,323]
    elif cols==3:widths=[112,182,174]
    else:widths=[WIDTH/cols]*cols
    data=[]
    for i,row in enumerate(rows):
        cells=[]
        for cell in row:
            if re.fullmatch(r'[a-f0-9]{64}',cell):
                text=f'<font name="Mono" size="7.2">{cell}</font>'
            else:text=inline(cell,small=True)
            cells.append(Paragraph(text,styles['th'] if i==0 else styles['cell']))
        data.append(cells)
    t=Table(data,colWidths=widths,repeatRows=1,hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),INK),('VALIGN',(0,0),(-1,-1),'TOP'),
        ('LEFTPADDING',(0,0),(-1,-1),8),('RIGHTPADDING',(0,0),(-1,-1),8),
        ('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[WHITE,PAPER]),
        ('LINEBELOW',(0,0),(-1,0),.7,TEAL),('LINEBELOW',(0,1),(-1,-1),.3,RULE)]))
    return t

def build():
    source=(HERE/'guide.md').read_text()
    # Readability only; leave identifiers, source hashes and filenames intact.
    for token in ['GNOME','Mutter','Shell','Clang','VM','CI']:
        source=re.sub(r'\b'+token+r'(?=\d)',token+' ',source)
    for a,b in [('mode0700','mode 0700'),('owned0700','owned 0700'),('all12','all 12'),('revision5','revision 5'),('child124','child 124'),('child0','child 0'),('exit0','exit 0'),('by proximity','by proximity')]:source=source.replace(a,b)
    lines=source.splitlines();story=[Cover(),PageBreak()];i=0
    while i<len(lines):
        line=lines[i].strip()
        if not line or line.startswith('# '):i+=1;continue
        if line=='[pagebreak]':story.append(PageBreak());i+=1;continue
        if line=='[toc]':
            story.append(Paragraph('Contents',ParagraphStyle('TOCTitle',parent=styles['h1'])))
            toc=TableOfContents();toc.levelStyles=[ParagraphStyle('TOC0',fontName='Sans',fontSize=8.8,leading=12.2,textColor=INK,leftIndent=0,firstLineIndent=0,spaceBefore=1),ParagraphStyle('TOC1',fontName='Sans',fontSize=8.1,leading=11,textColor=MUTED,leftIndent=15,firstLineIndent=0)]
            toc.tableStyle=TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),0),('RIGHTPADDING',(0,0),(-1,-1),0),('TOPPADDING',(0,0),(-1,-1),2),('BOTTOMPADDING',(0,0),(-1,-1),2)])
            story.append(toc);i+=1;continue
        if line.startswith('```'):
            code=[];i+=1
            while i<len(lines) and not lines[i].strip().startswith('```'):
                code.extend(textwrap.wrap(lines[i],100,subsequent_indent='    ',replace_whitespace=False,drop_whitespace=False) or ['']);i+=1
            story.append(Preformatted('\n'.join(code),styles['code']));i+=1;continue
        if line.startswith('[diagram:'):
            kind=line[9:-1];story.append(KeepTogether([Diagram(kind),Paragraph(CAPTIONS[kind],styles['caption'])]));i+=1;continue
        if line.startswith('### '):story.append(Paragraph(inline(line[4:]),styles['h2']));i+=1;continue
        if line.startswith('## '):
            story.append(CondPageBreak(135));story.append(Paragraph(inline(line[3:]),styles['h1']));i+=1;continue
        if line.startswith('|'):
            rows=[]
            while i<len(lines) and lines[i].strip().startswith('|'):
                row=[c.strip() for c in lines[i].strip().strip('|').split('|')]
                if not all(re.fullmatch(r':?-+:?',c) for c in row):rows.append(row)
                i+=1
            story.extend([Spacer(1,4),table(rows),Spacer(1,11)]);continue
        if line.startswith('> '):
            p=Paragraph(inline(line[2:]),styles['box'])
            t=Table([[p]],colWidths=[WIDTH]);t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,-1),PAPER),('LINEBEFORE',(0,0),(-1,-1),2,TEAL),('LEFTPADDING',(0,0),(-1,-1),12),('RIGHTPADDING',(0,0),(-1,-1),12),('TOPPADDING',(0,0),(-1,-1),10),('BOTTOMPADDING',(0,0),(-1,-1),10)]));story.extend([t,Spacer(1,12)]);i+=1;continue
        para=[line];i+=1
        while i<len(lines) and lines[i].strip() and not lines[i].startswith(('#','|','```','[diagram:','[pagebreak]','[toc]','> ')):
            para.append(lines[i].strip());i+=1
        text=' '.join(para);style=styles['ref'] if re.match(r'^\[(S|E)\d+\]',text) else styles['body']
        story.append(Paragraph(inline(text),style))
    OUT.parent.mkdir(parents=True,exist_ok=True)
    doc=GuideDoc(str(OUT),pagesize=(W,H),leftMargin=LEFT,rightMargin=LEFT,topMargin=72,bottomMargin=58,
        title='BSD³: A native desktop bridge for FreeBSD',author='Denis Lemire',
        subject='Architecture, implementation, reviewer navigation and experimental validation; hardware pending',
        pageCompression=1)
    frame=Frame(LEFT,58,WIDTH,H-72-58,id='body',leftPadding=0,rightPadding=0,topPadding=0,bottomPadding=0)
    doc.addPageTemplates(PageTemplate(id='Guide',frames=[frame],onPage=page))
    doc.multiBuild(story)
    manifest={'document':'BSD³ reviewer guide','author':'Denis Lemire','date':DATE,'implementation_reference':CODE_REF,
        'source_sha256':hashlib.sha256((HERE/'guide.md').read_bytes()).hexdigest(),
        'pdf_sha256':hashlib.sha256(OUT.read_bytes()).hexdigest(),'renderer':f'ReportLab {Version}',
        'fonts':list(fontfiles.values()),'status':'author-review draft; physical hardware validation pending'}
    (HERE/'build-manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
    print(OUT)

if __name__=='__main__':build()
