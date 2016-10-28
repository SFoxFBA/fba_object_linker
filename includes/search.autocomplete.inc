<?php

define('TABLE_ISLANDORA_AUTOCOMPLETE_SPARQL', 'islandora_autocomplete_sparql');
define('TABLE_ISLANDORA_AUTOCOMPLETE', 'islandora_autocomplete');

function fba_autocomplete_percent($string) {
  $fullGetRequest = $_GET['q'];
  $firstslash = strpos($fullGetRequest, '/') + 1;
  $lastslash = strrpos($fullGetRequest, '/');
  $autocomplete_string = substr($fullGetRequest, $firstslash, $lastslash - $firstslash);
  $query = substr($fullGetRequest, $lastslash + 1);
  $url = islandora_autocomplete_db_get_url_by_name($autocomplete_string);
  $url0 = $url[0];
  $index = isset($values['field']) ? $values['field'] - 1 : NULL;

  $results = $url0 ? islandora_autocomplete_query_source($url0->id, $url0->source, $query, $index) : array();
  $output = drupal_json_output($results);
  return $output;
}

/**
 * Autocomplete the content model name.
 *
 * @param string $param1
 *   A search string.
 *
 * @return string
 *   The rendered JSON results.
 */
function islandora_fba_autocomplete($param2, $name) {
  module_load_include('inc', 'islandora_autocomplete_sparql', 'includes/islandora_autocomplete_sparql.db');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  global $user;
  $query = '';
  $index = isset($values['field']) ? $values['field'] - 1 : NULL;
  $url = islandora_autocomplete_db_get_url_by_name($name);

  $results = $url[0] ? islandora_autocomplete_query_source($url[0]->id, $url[0]->source, $query, $index) : array();
  if ($json) {
    drupal_json($results);
    exit();
  }
  else {
    return $results;
  }

  /*
    $content_models = islandora_get_content_model_names();
    $output = array();
    foreach ($content_models as $model => $label) {
    if (preg_match("/{$param1}/i", $label) !== 0) {
    $output[$model] = $label;
    }
    }
    return drupal_json_output($output); */
}

/**
 * Fetches the given Autocomplete URL from the database.
 *
 * @param string $name
 *   The unique machine name for the requested Autocomplete URL.
 *
 * @return
 *   An object representing the given autocomplete url identified by name, or FALSE. The attributes
 *   of this object are the table fields selected by the query.
 */
function islandora_autocomplete_db_get_url_by_name($name) {
  $result = db_query('SELECT * FROM {' . TABLE_ISLANDORA_AUTOCOMPLETE . '} WHERE name = \'' . $name . '\'')->fetchAll();
  return $result; // !== FALSE ? db_fetch_object($result) : FALSE;
}

/**
 * Query the source.
 *
 * @param int $id
 *   The unique id for the requested Autocomplete URL.
 * @param string $source
 *   The source used to populate the autocomplete URL.
 * @param string $query
 *   The search query for the autocomplete.
 *
 * @return array
 *   The results of the query
 */
function islandora_autocomplete_query_source($id, $source, $query, $index = NULL) {
  $query_hook = "islandora_autocomplete_url_query_{$source}";
  $results = function_exists($query_hook) ? call_user_func($query_hook, $id, $query, $index) : array();
  return $results;
}

/**
 * Performs a Sparql query.
 *
 * Returns an alphabetically ordered list of values that match the query value.
 *
 * @param int $url_id
 *   The unique identifier for the Autocomplete URL.
 * @param string $query
 *   The value to insert into the Sparql query.
 *
 * @return array
 */
function islandora_autocomplete_url_query_sparql($url_id, $query = '', $index = NULL) {
  module_load_include('inc', 'islandora', 'includes/tuque');
  module_load_include('inc', 'islandora_autocomplete_sparql', 'includes/islandora_autocomplete_sparql.db');
  module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  global $user;
  $source = islandora_autocomplete_sparql_db_get_url_source($url_id);
  $source0 = $source[0];
  if ($source0) {
    // set variables
    $sparql_field = <<<EOQ
$source0->sparql_field
EOQ;
    $search_term_place_holder = 'search_term_place_holder';
    
    $sparql_field = str_replace($search_term_place_holder, $query, $sparql_field);
    // START OF PROCESSING REQUIRED FOR FIDORA
    $user_id = $user->islandora_user_pid;
    //$user_id = 'fba-user:61'; // Hard-coded temporary measure.
    $user_collection_place_holder = 'user_collection_place_holder';
    if (strlen($user_id) > 0 && strpos($sparql_field, $user_collection_place_holder)) {
      $user_collection_ns = str_replace(array('-', ':'), '', $user_id);
      $user_collection = 'info:fedora/' . $user_collection_ns . ':collection';
      $sparql_field = str_replace($user_collection_place_holder, $user_collection, $sparql_field);
    }
    // END OF PROCESSING REQUIRED FOR FIDORA
    $object_pid_append_required = $source0->object_pid_append_required;
    $comma_separated = $source0->comma_separated;

    // split query if comma separated autocompletion is enabled
    if ($comma_separated == 1) {
      // check for last comma
      $comma_pos = strrpos($query, ',');
      // if there is a comma found
      if ($comma_pos !== FALSE) {
        // split in 2 strings
        $query_prefix = trim(substr($query, 0, $comma_pos));
        $query = trim(substr($query, $comma_pos + 1));
      }
    }
    if (!empty($query)) {
      $query = Apache_Solr_Service::escape($query);
    }
    else {
      return $query;
    }
  }
  $limit = -1;
  $offset = 0;
  $sparql_field = str_replace("WHERE", "FROM <info:edu.si.fedora#ri> WHERE", $sparql_field); //needed to make fuseki queries work 
  //THIS WILL NEED ADDING TO NICKS SQL STATEMENTS! ^^^^
  $connection = new RepositoryConnection();
  $repquery = new RepositoryQuery($connection);

  $query_result = $repquery->sparqlQuery($sparql_field, $limit);
  //$query_result = ObjectHelper::performRiQuery($sparql_field, 'sparql', $limit, $offset);
  return islandora_autocomplete_sparql_parse_query_result($query_result, $object_pid_append_required);
}

/**
 * Fetches the given Autocomplete URL from the database.
 *
 * @param int $url_id
 *   The unique id for the requested Autocomplete URL.
 *
 * @return object
 *   An object representing the given autocomplete url
 *   sparql source identified by name, or FALSE.
 *   The attributes of this object are the table fields selected by the query.
 */
function islandora_autocomplete_sparql_db_get_url_source($url_id) {
  $result = db_query('SELECT * FROM {' . TABLE_ISLANDORA_AUTOCOMPLETE_SPARQL . '} WHERE url_id = ' . $url_id . ';')->fetchAll();
  return $result; // !== FALSE ? db_fetch_object($result) : FALSE;
}
