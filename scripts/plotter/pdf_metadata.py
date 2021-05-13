from PyPDF2 import PdfFileWriter, PdfFileReader

def setMetaData(in_file, out_file, metadata_dict):
    fin = open(in_file, 'rb')
    reader = PdfFileReader(fin)
    writer = PdfFileWriter()

    writer.appendPagesFromReader(reader)
    metadata = reader.getDocumentInfo()
    writer.addMetadata(metadata)

    # Write your custom metadata here:
    writer.addMetadata(metadata_dict)

    fout = open(out_file, 'wb')
    writer.write(fout)

    fin.close()
    fout.close()

#metadata_dict = {
#    '/Author' : 'json/...',
#    '/Creator' : 'BST plotter commit: XXX',
#    '/Producer' : 'Ivan Perez (University of Cantabria)'
#}

#setMetaData('source.pdf', 'result.pdf', metadata_dict)
