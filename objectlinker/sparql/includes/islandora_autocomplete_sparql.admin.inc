<?php

/**
 * @file
 * @author Nicholas Bywell of the Freshwater Biological Association (FBA).
 *
 * Functions for rendering/vaildating/submitting the Sparql subsection of the
 * autocomplet URL form.
 */

/**
 * Returns the form definition for the sparql source part of the
 * autocomplete url form.
 *
 * @param array $form_state
 *   The drupal form state.
 * @param int $url_id
 *   The id of the URL being edited, It is null when creating a new url.
 *
 * @return array
 *   The csv source part of the autocomplete url form.
 */
function islandora_autocomplete_sparql_form(array &$form_state, $url_id = NULL) {
  module_load_include('inc', 'islandora_autocomplete_sparql', 'includes/islandora_autocomplete_sparql.db');
  module_load_include('inc', 'islandora_autocomplete_sparql', 'includes/islandora_autocomplete_sparql.vocab');
  drupal_add_css(drupal_get_path('module', 'islandora_autocomplete_sparql') . '/css/islandora_autocomplete_sparql.admin.css');

  $options_relationship = fba_get_possible_relationships($object_pid);
  $value_relationship = !empty($form_state['values']['rdf_relationship']) ? $form_state['values']['rdf_relationship'] : "";


  // Gt db values.
  $row = islandora_autocomplete_sparql_db_get_url_source($url_id);
  $form = array();

  // Reset title.
  $form['#title'] = NULL;

  // Sparql field.
  $form['sparql_field'] = array(
    '#type' => 'textarea',
    '#required' => TRUE,
    '#description' => t('Enter a SPARQL query that will return the required values. If no RDF linking is required, the only mandatory component is that "?title" is included in the "SELECT" statement, a "FILTER regex(str(?title), "^search_term_place_holder", "i")" is included in the "WHERE" clause so that the string "search_term_place_holder" can be substituted with the characters typed in to the XML-form field by the user. If RDF linking is required, "?object" must be included along with "?title" in the "SELECT" statement in order to provide the ID of the object and the "Fields" field, if present above, should be set to "1". An example of a SPARQL query that provides a list of Preferred Vocabulary Terms is  --- SELECT ?title ?object WHERE { ?object &lthttp://purl.org/dc/elements/1.1/title&gt ?title; &ltfedora-model:state&gt &ltfedora-model:Active&gt;; &ltfedora-model:hasModel&gt &ltinfo:fedora/fba:vocabpref_CModel&gt;; &ltfedora-rels-ext:isMemberOfCollection&gt &ltinfo:fedora/fba:vocabpref_collection&gt FILTER regex(str(?title), "^search_term_place_holder", "i") } ORDER BY ?title --- '),
    '#title' => t('Sparql query'),
    '#default_value' => $row->sparql_field ? $row->sparql_field : '',
  );

  $form['rdf_relationship'] = array(
    '#type' => 'select',
    '#description' => t('If no RDF linking is required, leave this field set at "---Select---" and leave the fields below empty. If RDF linking is required, select the required relationship and you must also complete the following fields "Subject Datastream ID", "Subject Namespace Info", "Subject XPath Title Search" so that the title data can be found in the XML after the user submits the XML-form.'),
    '#title' => t('Relationship'),
    '#default_value' => $row->rdf_relationship ? $row->rdf_relationship : '',
    '#options' => $options_relationship,
  );

  $form['subject_dsid'] = array(
    '#type' => 'textfield',
    '#description' => t('If RDF linking is required, this is a mandatory field. Enter the ID of the datastream that the XML-form displays e.g. "MADS"'),
    '#title' => t('Subject Datastream ID'),
    '#default_value' => $row->subject_dsid ? $row->subject_dsid : '',
  );

  $form['subject_namespace_info'] = array(
    '#type' => 'textarea',
    '#description' => t('If RDF linking is required, this is a mandatory field. Enter the primary subject namespace prefix followed by the delimiter "!!", followed by the primary namespace URI for the subject datastream e.g.for a "MADS" datastream this would be "mads!!http://www.loc.gov/mads/v2". If more than one namespace is present in the datastream, add the other prefix/uri pairs in the same way, separating each by "=="'),
    '#title' => t('Subject Namespace Info'),
    '#default_value' => $row->subject_namespace_info ? $row->subject_namespace_info : '',
  );

  $form['subject_xpath_title_search'] = array(
    '#type' => 'textarea',
    '#description' => t('If RDF linking is required, this is a mandatory field. Enter as much of the XPath for the title as is present as a continuous string in the "Read"-"Path:" field of the XML-Form to which this autocomplete is to be attached, e.g. in the case of "FBA Vocab Form", this would bei the full "/mads:mads/mads:authority/mads:topic". However, in the case of the "Islandora PDF MODS Form" it would be "/mods:mods/mods:titleInfo" rather than the full "/mods:mods/mods:titleInfo/mods:title". The reason for this is that processing for this particular field involves string matching on the contents of the XML-Form rather than an XPath search on XML.'),
    '#title' => t('Subject XPath Title Search'),
    '#default_value' => $row->subject_xpath_title_search ? $row->subject_xpath_title_search : '',
  );

  $form['subject_xpath_data_search'] = array(
    '#type' => 'textarea',
    '#description' => t('If RDF linking is required, this is a mandatory field (unless there is no element corresponding to this relationship in the datastream and this autocomplete is only present so that the appropriate processing occurs on either a title change or a purge action, in which case this field should be left blank). Enter the XPath for the element in the datastream to which this autocomplete is to be attached, e.g. in the case of a MADS datastream,  for a broader term this would be "/mads:mads/mads:related[@type="broader"]/mads:topic", and for a related term this would be "/mads:mads/mads:related[not(@*)]/mads:topic"'),
    '#title' => t('Subject XPath Data Search'),
    '#default_value' => $row->subject_xpath_data_search ? $row->subject_xpath_data_search : '',
  );

  $form['object_pid_append_required'] = array(
    '#type' => 'checkbox',
    '#description' => t('If checked, the ID of the object is appended to the entry (prefixed and suffixed by twin vertical bars) so that identical entries can be distinguished. The IDs are automatically stripped out before the data is stored. If this option is not selected and there is more than one match, a link will not be created because the processing has no way to determine the correct match.'),
    '#title' => t('Append the object ID to distinguish identical entries?'),
    '#default_value' => $row->object_pid_append_required ? $row->object_pid_append_required : '',
  );

  $form['link_type'] = array(
    '#type' => 'select',
    '#description' => t('Select the type of linking required. If the user types in an entry because the required value is not present in the autocomplete list, and the "Object creation required" field is not checked, no link will be created.'),
    '#title' => t('Link type required'),
    '#default_value' => $row->link_type ? $row->link_type : '',
    '#options' => array( 0 => t('Outward only'), 1 => t('Outward and inverse'), 2 => t('Inverse only'), 3 => t('Literal')),
  );

  $form['object_creation_required'] = array(
    '#type' => 'checkbox',
    '#description' => t('If this is checked, if an object with a matching title does not already exist, it will be created. If you check this, the following fields must be completed, "Object Content Model PID", "Object Collection PID", "Object Namespace Prefix", "Object Datastream ID", "Object Namespace Info", "Object XPath Title Search", "Object Transform", "Object State" and "Object Datastream XML". The field "Object XPath Date of Creation Search" is also part of this group, but is optional.'),
    '#title' => t('Object creation required?'),
    '#default_value' => $row->object_creation_required ? $row->object_creation_required : '',
  );

  $form['symmetry_in_object_datastream_required'] = array(
    '#type' => 'checkbox',
    '#description' => t('If checked, a symmetric entry will be added to the datastream of the linked object. If this is checked, the following fields must be completed "Object Datastream ID", "Object Namespace Info", "Object XPath Data Search", "Object Pseudo-XPath Data Build" and "Object Pseudo-XPath Data Build Countback".'),
    '#title' => t('Add a symmetic entry in the object datastream?'),
    '#default_value' => $row->symmetry_in_object_datastream_required ? $row->symmetry_in_object_datastream_required : '',
  );

  $form['object_content_model_pid'] = array(
    '#type' => 'textfield',
    '#description' => t('Enter the PID of the Content Model associated with the objects that are to be linked to or, when applicable, created if they do not already exist e.g. "fba:vocabpref_CModel"'),
    '#title' => t('Object Content Model PID'),
    '#default_value' => $row->object_content_model_pid ? $row->object_content_model_pid : '',
  );

  $form['object_collection_pid'] = array(
    '#type' => 'textfield',
    '#description' => t('Enter the PID of the collection in which the object is created if it does not already exist e.g. "fba:vocabpref_collection"'),
    '#title' => t('Object Collection PID'),
    '#default_value' => $row->object_collection_pid ? $row->object_collection_pid : '',
  );

  $form['object_namespace_prefix'] = array(
    '#type' => 'textfield',
    '#description' => t('This field is mandatory if "Object creation required?" has been checked. Enter the namespace prefix for the objects that are created if they do not already exist e.g. "vocabpref"'),
    '#title' => t('Object Namespace Prefix'),
    '#default_value' => $row->object_namespace_prefix ? $row->object_namespace_prefix : '',
  );

  $form['object_dsid'] = array(
    '#type' => 'textfield',
    '#description' => t('This field is mandatory if either "Object creation required?" or "Add a symmetic entry in the object datastream?" has been checked. Enter the ID of the datastream of the inverse-object e.g."MADS".'),
    '#title' => t('Object Datastream ID'),
    '#default_value' => $row->object_dsid ? $row->object_dsid : '',
  );

  $form['object_namespace_info'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is mandatory if either "Object creation required?" or "Add a symmetic entry in the object datastream?" has been checked. Enter the primary object namespace prefix followed by the delimiter "!!" and then the primary namespace URI for the object datastream e.g.for a "MADS" datastream this would be "mads!!http://www.loc.gov/mads/v2". If more than one namespace is present in the datastream, add the other prefix/uri pairs in the same way, separating each by "==".'),
    '#title' => t('Object Namespace Info'),
    '#default_value' => $row->object_namespace_info ? $row->object_namespace_info : '',
  );

  $form['object_datastream_xml'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is mandatory if "Object creation required?" has been checked. A new object will be created with a datastream containing the XML entered in this field"'),
    '#title' => t('Object Datastream XML'),
    '#default_value' => $row->object_datastream_xml ? $row->object_datastream_xml : '',
  );

  $form['object_xpath_title_search'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is mandatory if "Object creation required?" has been checked. Enter the XPath for the title element in the XML that is entered in the "Object Datastream XML" field. For example, in the case of a MADS datastream, this would be "/mads:mads/mads:authority/mads:topic".'),
    '#title' => t('Object XPath Title Search'),
    '#default_value' => $row->object_xpath_title_search ? $row->object_xpath_title_search : '',
  );

  $form['object_xpath_date_create_search'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is relevant if "Object creation required?" is checked, but it is not mandatory. If entered it will result in the current date and time being entered into the specified XML element in a Solr-friendly format, e.g. "2014-04-22T14:36:23". Enter the XPath for the "Date of Creation" element in the XML that has been entered in the "Object Datastream XML" field. For example, in the case of a MADS datastream, this field would be "/mads:mads/mads:recordInfo/mads:recordCreationDate".'),
    '#title' => t('Object XPath Date of Creation Search'),
    '#default_value' => $row->object_xpath_date_create_search ? $row->object_xpath_date_create_search : '',
  );

  $form['object_transform'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is mandatory if "Object creation required?" has been checked. A new object will be created using the transform to Dublin Core that is specified in this field e.g. for a MADS datastream this might be "./sites/all/modules/islandora_content_model_forms/transforms/vocab_mads_to_dc.xsl"'),
    '#title' => t('Object Transform'),
    '#default_value' => $row->object_transform ? $row->object_transform : '',
  );

  $form['object_state'] = array(
    '#type' => 'textfield',
    '#description' => t('This field is mandatory if "Object creation required?" has been checked. A new object will be created with the state specified by this field. Enter either "A" for Active, "I" for Inactive.'),
    '#title' => t('Object State'),
    '#default_value' => $row->object_state ? $row->object_state : '',
  );

  $form['object_xpath_data_search'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is mandatory if "Add a symmetic entry in the object datastream?" has been checked. Enter the XPath for the element in the XML of the datastream in the object that provides symmetry. For example, for a broader term in the subject MADS datastream, the symmetrical equivalent in the object MADS datastream would be "/mads:mads/mads:related[@type="narrower]"/mads:topic", For the symmetrical equivalent of a related term in the subject MADS datastream (which has no "@type" attribute) this would be entered as "/mads:mads/mads:related[not(@*)]/mads:topic".'),
    '#title' => t('Object XPath Data Search'),
    '#default_value' => $row->object_xpath_data_search ? $row->object_xpath_data_search : '',
  );

  $form['object_xpath_data_build'] = array(
    '#type' => 'textarea',
    '#description' => t('This field is mandatory if "Add a symmetic entry in the object datastream?" has been checked. It requires a pseudo-XPath statement so that if the relevant node is either absent from the object datastream XML or already contains data, the processing can use this field to build the relevant node in order that it can be appended to provide symmetry. In most instances, this will be the same as the "Object XPath Data Search" fieldi, except that the namespace prefix should not be entered for each element. However, in those cases where the "Object XPath Data Search" contains a check for the non-existance of an attribute such as "/mads:mads/mads:related[not@*)]/mads:topic", the "Object XPath Data Build" field should contain only those components that are required for node creation e.g. "/mads/related/topic". Where more than one attribute is required in one node, the attributes should be separated by " and " as in "[@type="x" and @authority="y"]".'),
    '#title' => t('Object Pseudo-XPath Data Build'),
    '#default_value' => $row->object_xpath_data_build ? $row->object_xpath_data_build : '',
  );

  $form['object_xpath_data_build_countback'] = array(
    '#type' => 'textfield',
    '#description' => t('This field is mandatory if "Add a symmetic entry in the object datastream?" has been checked. Enter the number of nodes in the "Object XPath Data Build" pseudo-XPath that constitute a "node group" when this "node group" has to be appended to accommodate the symmetric data. In the case of an "Object Data XPath Build" entry of "/mads/related[@type="broader"]/topic", the value would be "2", because the nodes "/related[@type="broader"]/topic" would need to be appended. It is the number of nodes (delimited by "/") that have to be counted back from the end of the pseudo-XPath to achieve the required "node group".'),
    '#title' => t('Object Pseudo-XPath Data Build Countback'),
    '#default_value' => $row->object_xpath_data_build_countback ? $row->object_xpath_data_build_countback : '',
  );

  return $form;
}

/**
 * Validate the source portion of the submitted form.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 */
function islandora_autocomplete_sparql_form_validate(array $form, array &$form_state) {

}

/**
 * Submits the source portion of the form.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 */
function islandora_autocomplete_sparql_form_submit(array $form, array &$form_state) {
  // Don't use sumbit functions when ahah is called.
  if ($form_state['ahah_submission'] !== TRUE) {
    module_load_include('inc', 'islandora_autocomplete_sparql', 'includes/islandora_autocomplete_sparql.db');
    //extract($form_state['values'], EXTR_SKIP);

    $sparql_field = $form_state['values']['sparql_field'];
    $url_id = $form_state['values']['url_id'];
    $rdf_relationship = $form_state['values']['rdf_relationship'];
    $object_pid_append_required = $form_state['values']['object_pid_append_required'];
    $subject_dsid = $form_state['values']['subject_dsid'];
    $subject_namespace_info = $form_state['values']['subject_namespace_info'];
    $subject_xpath_title_search = $form_state['values']['subject_xpath_title_search'];
    $subject_xpath_data_search = $form_state['values']['subject_xpath_data_search'];
    $object_creation_required = $form_state['values']['object_creation_required'];
    $link_type = $form_state['values']['link_type'];
    $symmetry_in_object_datastream_required = $form_state['values']['symmetry_in_object_datastream_required'];
    $object_dsid = $form_state['values']['object_dsid'];
    $object_namespace_info = $form_state['values']['object_namespace_info'];
    $object_datastream_xml = $form_state['values']['object_datastream_xml'];
    $object_xpath_title_search = $form_state['values']['object_xpath_title_search'];
    $object_xpath_date_create_search = $form_state['values']['object_xpath_date_create_search'];
    $object_content_model_pid = $form_state['values']['object_content_model_pid'];
    $object_collection_pid = $form_state['values']['object_collection_pid'];
    $object_namespace_prefix = $form_state['values']['object_namespace_prefix'];
    $object_transform = $form_state['values']['object_transform'];
    $object_state = $form_state['values']['object_state'];
    $object_xpath_data_search = $form_state['values']['object_xpath_data_search'];
    $object_xpath_data_build = $form_state['values']['object_xpath_data_build'];
    $object_xpath_data_build_countback = $form_state['values']['object_xpath_data_build_countback'];


    // Sparql field.
    if (!empty($sparql_field)) {
      // File already existed.
      if (isset($url_id)) {
        $source = islandora_autocomplete_sparql_db_get_url_source($url_id);
        // Create values array.
        $values = array(
          'sparql_field' => $sparql_field ? $sparql_field : '',
          'rdf_relationship' => $rdf_relationship ? $rdf_relationship : '',
          'object_pid_append_required' => $object_pid_append_required,
          'subject_dsid' => $subject_dsid ? $subject_dsid : '',
          'subject_namespace_info' => $subject_namespace_info ? trim($subject_namespace_info) : '',
          'subject_xpath_title_search' => $subject_xpath_title_search ? trim($subject_xpath_title_search) : '',
          'subject_xpath_data_search' => $subject_xpath_data_search ? trim($subject_xpath_data_search) : '',
          'object_creation_required' => $object_creation_required,
          'link_type' => $link_type,
          'symmetry_in_object_datastream_required' => $symmetry_in_object_datastream_required,
          'object_dsid' => $object_dsid ? trim($object_dsid) : '',
          'object_namespace_info' => $object_namespace_info ? trim($object_namespace_info) : '',
          'object_datastream_xml' => $object_datastream_xml ? trim($object_datastream_xml) : '',
          'object_xpath_title_search' => $object_xpath_title_search ? trim($object_xpath_title_search) : '',
          'object_xpath_date_create_search' => $object_xpath_date_create_search ? trim($object_xpath_date_create_search) : '',

          'object_content_model_pid' => $object_content_model_pid ? trim($object_content_model_pid) : '',
          'object_collection_pid' => $object_collection_pid ? trim($object_collection_pid) : '',
          'object_namespace_prefix' => $object_namespace_prefix ? trim($object_namespace_prefix) : '',
          'object_transform' => $object_transform ? trim($object_transform) : '',
          'object_state' => $object_state ? trim($object_state) : '',
          'object_xpath_data_search' => $object_xpath_data_search ? trim($object_xpath_data_search) : '',
          'object_xpath_data_build' => $object_xpath_data_build ? trim($object_xpath_data_build) : '',
          'object_xpath_data_build_countback' => $object_xpath_data_build_countback ? trim($object_xpath_data_build_countback) : ''
        );
        // Source exists, update it.
        if ($source) {
          islandora_autocomplete_sparql_db_update_url_source($source->id, $url_id, $values);
          // Updated Source.
          return;
        }
        // Add new source.
        islandora_autocomplete_sparql_db_add_url_source($url_id, $values);
      }
    }
  }
}

/**
 * islandora autocomplete sparql ahah callback
 */
function islandora_autocomplete_sparql_admin_ahah() {

  // Include original form file.
  module_load_include('inc', 'islandora_autocomplete', 'includes/islandora_autocomplete.admin');
  // Generic AHAH fiddling.
  $form_state = array('storage' => NULL, 'submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  $form = form_get_cache($form_build_id, $form_state);
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  $form_state['post'] = $form['#post'] = $_POST;
  // Enable the submit/validate handlers to determine whether AHAH-submittted.
  $form_state['ahah_submission'] = TRUE;
  $form['#programmed'] = $form['#redirect'] = FALSE;
  drupal_process_form($form_id, $form, $form_state);
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);

  // Render the output.
  //  $output = theme('status_messages');
  $output = drupal_render($ngram_wrapper);

  // Final rendering callback.
  drupal_json(array('status' => TRUE, 'data' => $output));

  exit();

}

function relationship_ajax_callback(&$form, $form_state) {

  return array(
    '#type' => 'ajax',
    '#commands' => array(
      	ajax_command_replace("#textfield_description_replace", render($form['rels']['description'])),
	ajax_command_replace("#fieldset_symm_replace", render($form['symmetric'])),
	ajax_command_replace("#fieldset_ns_replace", render($form['ns']))
    )
  );
}
