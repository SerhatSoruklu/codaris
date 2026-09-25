#!/usr/bin/env python3
"""Import owner-supplied research packs, preserving originals and workbook cells."""
import hashlib
import json
import posixpath
import shutil
import sys
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
NS={'s':'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
def workbook_tables(path):
    with zipfile.ZipFile(path) as z:
        strings=[]
        if 'xl/sharedStrings.xml' in z.namelist():
            strings=[''.join(n.itertext()) for n in ET.fromstring(z.read('xl/sharedStrings.xml'))]
        rels={r.attrib['Id']:r.attrib['Target'] for r in ET.fromstring(z.read('xl/_rels/workbook.xml.rels'))}
        tables={}
        for sheet in ET.fromstring(z.read('xl/workbook.xml')).findall('s:sheets/s:sheet',NS):
            target=rels[sheet.attrib['{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id']]
            target=target.lstrip('/') if target.startswith('/') else posixpath.normpath('xl/'+target)
            rows=[]
            for row in ET.fromstring(z.read(target)).findall('.//s:row',NS):
                vals=[]
                for cell in row:
                    letters=''.join(c for c in cell.attrib.get('r','') if c.isalpha()); index=0
                    for c in letters:index=index*26+ord(c)-64
                    while len(vals)<index:vals.append('')
                    v=cell.find('s:v',NS);kind=cell.attrib.get('t')
                    value=strings[int(v.text)] if kind=='s' and v is not None else ''.join(cell.find('s:is',NS).itertext()) if kind=='inlineStr' else (v.text or '') if v is not None else ''
                    if index:vals[index-1]=value
                while vals and not vals[-1]:vals.pop()
                if vals:rows.append(vals)
            tables[sheet.attrib['name']]=rows
        return tables
if __name__=='__main__':
    source=Path(sys.argv[1]); manifests=[];checksums=[]
    for catalog in sorted(source.rglob('*_catalog.json')):
        raw=json.loads(catalog.read_text(encoding='utf-8'))
        number=raw['metadata'].get('codaris_category_number',4)
        slug={4:'runtimes',5:'package-build-tools',6:'developer-environments',7:'version-control',8:'testing-qa',9:'cloud-hosting',10:'containers-infrastructure',11:'cicd-automation',12:'servers-networking',13:'messaging-streaming',14:'observability',15:'apis-auth-integration',16:'ai-developer-tools'}[number]
        dest=ROOT/'data/topics'/slug;dest.mkdir(parents=True,exist_ok=True);source_files=[]
        for f in sorted(catalog.parent.iterdir()):
            if f.is_file() and f.suffix in ('.json','.csv','.xlsx','.md'):
                shutil.copyfile(f,dest/f.name)
                checksums.append(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {slug}/{f.name}')
                source_files.append(f.name)
        workbook=next(dest.glob('*.xlsx'))
        (dest/'workbook-tables.json').write_text(json.dumps(workbook_tables(workbook),ensure_ascii=False,indent=2)+'\n')
        title=raw['metadata'].get('category','Runtimes & Execution Platforms')
        manifests.append({'number':number,'slug':slug,'title':title,'catalog':catalog.name,'count':raw['metadata']['catalogue_count'],'source_files':source_files})
    assert len(manifests)==13
    manifests.sort(key=lambda x:x['number'])
    (ROOT/'data/topics/manifest.json').write_text(json.dumps(manifests,indent=2)+'\n')
    (ROOT/'data/topics/SHA256SUMS').write_text('\n'.join(sorted(checksums))+'\n')
    dest=ROOT/'data/topics/source-manifest';dest.mkdir(exist_ok=True)
    for f in (source/'CODARIS_remaining_12_research').iterdir():
        if f.is_file():shutil.copyfile(f,dest/f.name)
    print('Imported',len(manifests),'packs;',sum(m['count'] for m in manifests),'rows; originals hashed.')
