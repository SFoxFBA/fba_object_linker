<?php

/**
 * @file
 *
 */
module_load_include('inc', 'fedora_repository', 'api/fedora_item');
module_load_include('inc', 'fedora_repository', 'ContentModel');
module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
module_load_include('inc', 'xml_form_api', 'XMLForm');

/**
 * Create an ingest form.
 * FIXME:  This is just a collection of functions; there is no object state.
 *   All functions should be made static and so accessed.
 */
class IngestObjectMetadataForm {

  /**
   * Get Content Models for the form.
   *
   * @param string $collection_pid
   *
   * @return array
   */
  public function getPotentialContentModels($collection_pid, array &$form_state) {
    $potential_models = array();

    //Add models from collection policy.
    if (($collection_policy = CollectionPolicy::loadFromCollection($collection_pid)) !== FALSE && ($content_models = $collection_policy->getContentModels())) {
      $potential_models = array();
      foreach ($content_models as $content_model) {
        $identifier = $content_model->getIdentifier();
        $name = $content_model->name;
        $potential_models["$identifier"] = "$name";
      }
    }

    // Add cmodels to programatically include...
    if ($CMs = $form_state['storage']['content_model']) {
      $CMs = (array) $CMs;
      module_load_include('inc', 'fedora_repository', 'ContentModel');
      foreach ($CMs as $content_model) {
        $content_model = ContentModel::loadFromModel($content_model);
        $identifier = $content_model->getIdentifier();
        $name = $content_model->name;
        $potential_models["$identifier"] = "$name";
      }
    }

    return $potential_models;
  }

  /**
   * Create the first page of the Ingest Object Metadata form.
   *
   * @note
   *   this code contains a temporary fix and should be replaced as soon as a more permananent solution is developed
   * currently if a collection has more than one content model listed in the collection policy the user
   * will only have access to the first form associated with the content type.
   *
   * @param string $collection_pid
   * @param string $collection_label
   * @param array $form_state
   *
   * @return array
   */
  public function createPageOne($collection_pid, $collection_label, array &$form_state) {

    $potential_models = $this->getPotentialContentModels($collection_pid, $form_state);
    unset($potential_models['islandora:collectionCModel/ISLANDORACM']);
    list($identifier, $name) = array_peek($potential_models);
    $selected_model = isset($form_state['values']['models']) ? $form_state['values']['models'] : $identifier;
    $identifier = isset($form_state['post']['models']) ? $form_state['post']['models'] : $identifier;
    $content_model_pid = ContentModel::getPidFromIdentifier($identifier);
    $form_names = islandora_content_model_get_form_names($content_model_pid);

    $form = array();

    if (count($potential_models) == 0) {
      if (!user_access('manage collections')) {
        $message = t('This collection cannot receive additions.  Please contact the site administrator if you believe this message to be in error.');
      }
      else {
        if (module_exists('islandora_collection_manager')) {
          // XXX: Should get the value from the collection manager?
          $message = t('Select the %manage tab to add content models or child collections to this collection', array('%manage' => 'Manage This Collection'));
        }
        else {
          $collection_link = l("Islandora Collection Manager", "https://wiki.duraspace.org/display/ISLANDORA6121/Collection+Manager");
          $message = t('This collection requires either one or more content models in its COLLECTION_POLICY, or one or more child collections.<br />
                      We strongly recommend enabling the !collection_link module for managing collection objects.', array('!collection_link' => $collection_link));
        }
      }
      $form['message'] = array(
        '#type' => 'item',
        '#value' => $message,
      );
      return $form;
    }

    $form['indicator'] = array(
      '#type' => 'fieldset',
      '#attributes' => array('class' => 'fedora_ingester'),
      '#id' => 'fedora_ingest_fieldset',
      '#title' => t('Ingest digital object into %collection_label (@collection_pid), Step #1:', array('@collection_pid' => $collection_pid, '%collection_label' => $collection_label)),
    );

    $form['indicator']['models'] = array(
      '#type' => 'select',
      '#title' => t('Content models available'),
      '#options' => $potential_models,
      '#default_value' => $selected_model,
      '#description' => t('Content models define datastream composition, relationships between this and other content models, and the mandatory behaviors associated with each digital object.<br /> Additional information may be found <a href="https://wiki.duraspace.org/display/FEDORACREATE/Content+Models+Overview">here.</a> '),
      '#ahah' => array(
        'path' => 'islandora/form/update',
        'wrapper' => 'content-form-choice',
        'effect' => 'fade',
      ),
    );

    $form['indicator']['content_form_ahah_wrapper'] = array(
      '#prefix' => '<div id="content-form-choice">',
      '#suffix' => '</div>',
      '#type' => 'item',
    );

    if (count($form_names) == 1) {
      $form['indicator']['content_form_ahah_wrapper']['forms'] = array(
        '#type' => 'hidden',
        '#value' => array_shift(array_keys($form_names))
      );
    }
    elseif (count($form_names) > 0) {
      $form['indicator']['content_form_ahah_wrapper']['forms'] = array(
        '#type' => 'select',
        '#title' => t('Select form'),
        '#options' => $form_names,
        '#description' => t('Select the form to populate the metadata of the new object.'),
      );
    }

    $form['collection_pid'] = array(
      '#type' => 'hidden',
      '#value' => $collection_pid
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#submit' => array('fedora_repository_ingest_form_submit'),
      '#value' => t('Next'),
    );
    return $form;
  }

  /**
   * Create the second page of the Ingest Object Metadata form.
   *
   * @param string $collection_pid
   * @param string $collection_label
   * @param array $form_state
   *
   * @return array
   */
  public function createPageTwo($collection_pid, $collection_label, array &$form_state) {
    //module_load_include('inc', 'fedora_repository', 'formClass');
    //$path = drupal_get_path('module', 'islandora_content_model_forms');
    $content_model_pid = substr($form_state['values']['models'], 0, strpos($form_state['values']['models'], '/'));
    $content_model_dsid = substr($form_state['values']['models'], strpos($form_state['values']['models'], '/') + 1);

    $form_name = NULL;
    if (isset($form_state['values']['forms'])) {
      $form_name = $form_state['values']['forms'];
    }
    else {
      $form_names = sidora_get_form_and_dsid($content_model_pid);
      //$form_names = islandora_content_model_get_form_names($content_model_pid);
      reset($form_names);
      $form_name = key($form_names);
      $form_state['values']['forms'] = $form_name;
    }
    if (empty($form_name)) {
      $ingestForm = new formClass();
      $form_state['storage']['content_model'] = $content_model;
      $form_state['storage']['collection_pid'] = $collection_pid;
      return $ingestForm->createQDCIngestForm($collection_pid, $collection_label, $form_state);
    }
    $dsid = $form_state['values']['dsid'];

    $xml = NULL;
    if (!empty($form_state['storage']['xml_data'])) {
      $xml = $form_state['storage']['xml_data'];
    }

    $form = xml_form_builder_get_form(array($form_name), $form_state, $form_name, $xml);
    $form['collection_pid'] = array(
      '#type' => 'hidden',
      '#value' => $collection_pid
    );
    $form['content_model_pid'] = array(
      '#type' => 'hidden',
      '#value' => $content_model_pid
    );
    $form['content_model_dsid'] = array(
      '#type' => 'hidden',
      '#value' => $content_model_dsid
    );
    $form['dsid'] = array(
      '#type' => 'hidden',
      '#value' => $dsid
    );
    $form['models'] = array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['models']
    );
    $form['forms'] = array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['forms']
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#attributes' => array('class' => 'fedora_ingest_page_two_submit'),
      '#value' => t('Submit'),
    );

    return $form;
  }

  /**
   * Create the Ingest Object Metadata form.
   *
   * @param string $collection_pid
   * @param string $collection_label
   * @param array $form_state
   *
   * @return array
   */
  public function create($collection_pid, $collection_label, array &$form_state) {
    $page = $form_state['storage']['step'] = empty($form_state['storage']['step']) ? 1 : $form_state['storage']['step'];

    if ($page == 1) {
      $form_state['storage']['xml'] = TRUE;
      return $this->createPageOne($collection_pid, $collection_label, $form_state);
    }
    else if ($form_state['storage']['xml']) {
      return $this->createPageTwo($collection_pid, $collection_label, $form_state);
    }
    else {
      throw new Exception("Couldn't Create the Ingest Form.");
    }
  }

  /**
   * Ingest the object.
   *
   * @global string $base_url
   *
   * @param array $form
   * @param array $form_state
   */
  public function submit($form, $form_state, $pid, $association, $document) {
    module_load_include('inc', 'fba_object_linker', 'objectlinker/ObjectLinker');

    $collection_pid = $form_state['build_info']['args'][0]['collection']->id;


    $content_model_pid = $association['content_model'];
    $content_model_dsid = $association['dsid'];
    $dsid = $association['dsid'];
    $transform = $association['transform'];
    //$state = $association[''];
    $relationship = NULL;

    if (empty($relationship)) {
      if (!empty($form_state['storage']['parent_relation'])) {
        $relationship = $form_state['storage']['parent_relation'];
      }
    }


    /**
     *
     * FBA MODIFICATION BY NBYWELL on 06/03/2014. Check content of
     * the 'rdf_relationships' variable which triggers the
     * Object Linker functionality.
     *
     */
    $ocrProcessingRequired = false;
    $ocrProcessingActivated = false; // This variable will not longer be required once the processing has been fully tested.
    $object = islandora_object_load($pid);

    $label = $object->label;
    if (empty($label)) {
      $label = "Undefined";
    }

    module_load_include('inc', 'fba_object_linker', 'objectlinker/ObjectLinker');
    $rdfrels = variable_get('rdf_relationships', NULL);
    if (is_null($rdfrels)) {
      /*
       * Create Object-Object links.
       */
      process_links($form, $form_state, false, $object->id);
    }
    // The ocrProcessingActivated condition has been included below
    // to disable this functionality until it has been fully tested.
    if ($ocrProcessingRequired && $ocrProcessingActivated) {
      /*
       * Extract the text from the PDF in the temporary directory.
       * directory, add it as an OCR datastream and then
       * remove the temporary directory.
       */
      extractOcrFromPdf($pid, $pdfFilePath, $escapedPdfFile, $escapedPdfDirPath, $tmpDir, $tmpDirPath, $concatOutputFilePath, $ocrMsg);
    }
    $_SESSION['fedora_ingest_files'] = '';
    $form_state['storage'] = NULL;


    //$form_state['redirect'] = "fedora/repository/{$collection_pid}";
  }

  function islandora_content_model_update_form_choice() {
// prep only
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

    $changed_elements = $form['indicator']['content_form_ahah_wrapper'];
    unset($changed_elements['#prefix'], $changed_elements['#suffix']);
    $output = theme('status_messages') . drupal_render($changed_elements);
    drupal_json(array(
      'status' => TRUE,
      'data' => $output,
    ));
  }

  /**
   * Constructs a new CollectionPolicy object from the specified 
   * collection PID.  If preFetch is disabled, then Islandora will not get the datastream until needed. 
   * (useful when cacheing)
   * Returns FALSE on failure.
   *
   * @param string $pid
   * @param boolean $preFetch = TRUE
   * @return CollectionPolicy $ret
   */
  function loadFromCollection($pid, $preFetch = TRUE) {
    $ret = FALSE;
    //module_load_include('inc', 'fedora_repository', 'api/fedora_item');

    try {
      $dsid = getDefaultDSID();

      if ($preFetch) {
        $connection = islandora_get_tuque_connection();
        $object = $connection->repository->getObject($pid);
        //$fedoraItem = new Fedora_Item($pid);
        $ds = $object->getDatastream($dsid);
      }
      else {
        $ds = NULL;
      }
      if (!empty($ds) || !$preFetch) {
        $ret = new CollectionPolicy($ds, $pid, $dsid);
      }
    }
    catch (SOAPException $e) {
      $ret = FALSE;
    }
    return $ret;
  }

  /**
   * Gets the default DSID to use for ContentModel datastreams.
   *
   * @return string $default_dsid
   */
  function getDefaultDSID() {
    return variable_get('Islandora_Collection_Policy_DSID', 'COLLECTION_POLICY');
  }

  /**
   *
   * @param string $content_model_pid
   * @param string $form_name
   * @return string 
   */
  function islandora_content_model_get_transform($content_model_pid, $form_name) {
    $associations = xml_form_builder_get_associations(array($form_name), array(), array(), FALSE);
    //$result = db_query("SELECT transform FROM {islandora_content_model_forms} where content_model = :custom_url AND form_name = :form_name", array(':custom_url' => $content_model_pid, ':form_name' => $form_name));
    foreach ($associations as $data) {
      if (isset($data['transform'])) {
        $xsl_file = "./{$data['transform']}";

        $old_transform_path = drupal_get_path('module', 'sidora') . "/transforms/{$data['transform']}";
        if (!file_exists($xsl_file)) {
          $xsl_file = "./$old_transform_path";

          if (file_exists($xsl_file)) {
            watchdog('islandora_content_model_forms', 'Found the transform "@transform" in the legacy location. Transforms ' .
                'should be specified by a path relative to the server\'s document root.', array(
              '@transform' => $data['transform'],
              '@old_dir' => $old_transform_path,
                ), WATCHDOG_WARNING);
          }
          else {
            watchdog('islandora_content_model_forms', 'The XSLT transform to DC cannot be located as either "@transform"' .
                ' or "@old_dir"!.', array(
              '@transform' => $data['transform'],
              '@old_dir' => $old_transform_path,
                ), WATCHDOG_ERROR);
            return NULL;
          }
        }
        return $xsl_file;
      }
    }
    return NULL;
  }

}
