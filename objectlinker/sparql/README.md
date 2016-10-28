"Sparql Submodule" and the "FBA Object Linker Module"
===================================================== 

This README file is for the "Sparql Submodule" of the "Islandora Autocomplete
Module" .The "Sparql Submodule" is just one component of the "FBA Object Linker
Module", and because the code for the "FBA Object Linker Module" is spread
across a number of other existing modules, this README file primarily serves as
the README file for that module.

The "FBA Object Linker Module" uses some of the ontology processing developed
by Giancoralo Birello of CNR-Ceris and Rosemary Le Faive of McGill University
in the Islandora Relationship Editor module.

Prerequisites for the FBA Object Linker Module:

Islandora-6.x-13.1
Tuque-1.x
PHP 5.3.3+
arc2

Installation Instructions:

To install the "Sparql Submodule", download the islandora_autocomplete-6.x.zip file
that can be found at:

https://github.com/FBA/islandora_autocomplete

If you do not already have the "Islandora Autocomplete Module" loaded,
just unzip the file in the "/sites/all/modules/ folder and enable it
and its submodules in the "/admin/build/modules" section of the
Administrator Module.

If you already have the "Islandora Autocomplete Module"
enabled on your system, unzip the file in a temporary location and zip the
contents of only the sparql folder and unzip that in the
"/sites/all/modules/islandora_autocomplete/modules" folder.

Download the zip file from https://github.com/semsol/arc2, unzip it and copy the arc2 folder into the "modules/sparql/includes" folder.

Enable the sparql submodule in the "/admin/build/modules" section
of the Administration Module.
Module.

To install the rest of the "FBA Object Linker" Module:

Download and replace the following existing files:

https://github.com/FBA/islandora/blob/6.x/fedora_repository.module
https://github.com/FBA/islandora_collection_manager/blob/6.x/management/DeleteCollection.inc
https://github.com/FBA/islandora_xml_forms/blob/6.x/api/XMLFormDefinition.inc
https://github.com/FBA/islandora_content_model_forms/blob/6.x/EditObjectMetadataForm.inc
https://github.com/FBA/islandora_content_model_forms/blob/6.x/IngestObjectMetadataForm.inc
https://github.com/FBA/islandora_solution_pack_pdf/blob/6.x/pdf_sp.inc
https://github.com/FBA/islandora_solution_pack_pdf/blob/6.x/xml/mods_pdf.xml

Download the following new file:

https://github.com/FBA/islandora_content_model_forms/blob/6.x/ObjectLinker.inc

Replace the existing XML-Form "Islandora PDF MODS Form" by using
the "/admin/content/xml/form" functionality in the Administrator Module to
import the mods_pdf.xml file.

The reason that the mods_pdf.xml file has changed is because the "Topic" and
"Geographic" fields are tag-type fields which do not permit the type of
autocomplete functionality that is required for the Object Linker Module
processing. The "Topic", "Geographic" and "Temporal" fields have therefore
been converted into the tab-panel type fields. This has siginificant
implications if records are already present in the collection. XML in tag-type
format will not work with the XML produced by tab-panels. If the modified form
is to be adopted for use with real data, a batch transform would be required
to convert the relevant XML elements in the MODS datastreams in affected
records into the tab-panel format.

If you have enforced namespace restrictions in
"/admin/settings/fedora_repository", make "fba" a permitted PID namespace.

Using the "Fedora-Admin Utility", create a new object in the fedora
repository with:
"Custom PID" set to "fba:ontology"
"Label" set to "FBA Ontology"

Create a new datastream in this object with the following settings: 
"ID" of "ONTOLOGY"

"Control Group" of "Internal XML Metadata"
,
"State" of "Active"

"MIME Type" of "text/xml"

"Label" of "ONTOLOGY"

"Checksum" set to "DISABLED"

Import into the new datastream the XML contained in the file:
/islandora_autocomplete/modules/sparql/xml/mads_plus_fba_ontology.rdf

The mads_plus_fba_ontology.xml file contains a combination of MADS-RDF
statements and RDF statements created by the FBA.

This completes the installation of the "FBA Object Linker Module". The
"FBA Vocabulary Module" can now be installed if it is required. A README
file with instructions on how to do this can be found at:
https://github.com/FBA/fba_solution_pack_vocabulary/README.md

END
