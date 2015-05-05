<?php

/**
 * @file
 * Functions for pushing public questions to the Well.
 */

/**
 * Publish question.
 */
function vopros_public_question_publish_question($public_question) {
  return _vopros_public_question_publish_questions(array('create' => array($public_question)));
}

/**
 * Update published question.
 */
function vopros_public_question_update_question($public_question) {
  return _vopros_public_question_publish_questions(array('update' => array($public_question)));
}

/**
 * Delete published question.
 */
function vopros_public_question_delete_question($public_question) {
  return _vopros_public_question_publish_questions(array('delete' => array($public_question)));
}

/**
 * Handle talking to the Well.
 */
function _vopros_public_question_publish_questions($actions) {
  $agency  = variable_get('ting_agency', '');
  $source = variable_get('vopros_public_question_source', variable_get('osa_addi_source', ''));
  $o_option = variable_get('vopros_public_question_o_option', '');
  $mail = variable_get('vopros_public_question_mail', '');
  $ftp = variable_get('vopros_public_question_ftp', '');

  $file_base = $agency . '.' . REQUEST_TIME;

  // Prepare XML.
  $namespaces = array(
    'xsi' => 'http://www.w3.org/2001/XMLSchema-instance',
    'dc' => 'http://purl.org/dc/elements/1.1/',
    'dkabm' => 'http://biblstandard.dk/abm/namespace/dkabm/',
    'dcterms' => 'http://purl.org/dc/terms/',
    'ac' => 'http://biblstandard.dk/ac/namespace/',
    'dkdcplus' => 'http://biblstandard.dk/abm/namespace/dkdcplus/',
    'docbook' => 'http://docbook.org/ns/docbook',
  );

  $doc = new DOMDocument('1.0', 'UTF-8');
  $collection = $doc->createElementNS('http://biblstandard.dk/abm/namespace/dkabm/', 'collection');
  $doc->appendChild($collection);

  foreach ($actions as $action => $public_questions) {
    foreach ($public_questions as $public_question) {
      _vopros_public_question_request_object($public_question, $doc, $collection, $source, $action);
    }
  }

  $doc->formatOutput = TRUE;
  $xml = $doc->saveXml();

  // Prepare transfer file.
  $transfer = "b=databroendpr2,f=$file_base.xml,t=xml,o=$o_option,c=utf8,m=$mail\n";
  $transfer .= "slut\n";

  $transfer_filename = $file_base . '.trans';
  $xml_filename = $file_base . '.xml';
  if (empty($ftp)) {
    // This makes things easier to debug.
    drupal_set_message(t("<pre>Transfer file @filename: \n@content</pre>",
                         array(
                           '@filename' => $transfer_filename,
                           '@content' => $transfer,
                         )));
    drupal_set_message(t("<pre>XML file @filename: \n@content</pre>",
                         array(
                           '@filename' => $xml_filename,
                           '@content' => $xml,
                         )));
  }
  else {
    $ftp = 'ftp://' . rtrim($ftp, '/') . '/';
    if (file_put_contents($ftp . $xml_filename, $xml) === FALSE) {
      drupal_set_message(t('Could not upload XML file.'), 'error');
      return FALSE;
    }
    if (file_put_contents($ftp . $transfer_filename, $transfer) === FALSE) {
      drupal_set_message(t('Could not upload trans file.'), 'error');
      return FALSE;
    }
  }

  return TRUE;
}

/**
 * Generate the XML for a public question.
 */
function _vopros_public_question_request_object($object, $doc, $collection, $source, $action = 'create') {
  $actions = array(
    'create' => 'created',
    'update' => 'modified',
    'delete' => 'delete-out-of-scope',
  );

  if (!isset($actions[$action])) {
    return;
  }

  $container = $doc->createElementNS('http://biblstandard.dk/abm/namespace/dkabm/', 'record');

  // Create the record.
  $dkabm_record = $doc->createElement('dkabm:record');
  // Add values to the record.
  $dkabm_record->appendChild($doc->createElement('ac:identifier', $object->uuid));
  $dkabm_record->appendChild($doc->createElement('dc:date', date('Y', $object->created)));

  // Add record to the container.
  $container->appendChild($dkabm_record);

  $activity = $doc->createElement('ac:activity');
  $action_elem = $doc->createElement('ac:action', $actions[$action]);
  $action_elem->setAttribute('xsi:type', "ac:TypeOfActivity");
  $activity->appendChild($action_elem);
  $dkabm_record->appendChild($activity);

  // The rest is not needed for delete requests.
  if ($action != 'delete') {
    $dkabm_record->appendChild($doc->createElement('ac:source', $source));
    $dc_identifier = $doc->createElement('dc:identifier', "http://biblioteksvagten.dk/single_question.asp?quid=" . $object->uuid);
    $dc_identifier->setAttribute('xsi:type', "dcterms:URI");
    $dkabm_record->appendChild($dc_identifier);
    $dkabm_record->appendChild($doc->createElement('dc:title', str_replace('&', '&amp;', $object->title)));
    if (is_array($object->subject)) {
      foreach ($object->subject as $subject) {
        $dkabm_record->appendChild($doc->createElement('dc:subject', str_replace('&', '&amp;', $subject)));
      }
    }
    $dkabm_record->appendChild($doc->createElement('dc:description', str_replace('&', '&amp;', $object->description)));
    $dc_type = $doc->createElement('dc:type', 'Spørgsmål og svar');
    $dc_type->setAttribute('xsi:type', 'dkdcplus:BibDK-Type');
    $dkabm_record->appendChild($dc_type);
    $dkabm_record->appendChild($doc->createElement('dc:language', 'Dansk'));

    $dkabm_record->appendChild($doc->createElement('dcterms:created', date('c', $object->created)));
    $dkabm_record->appendChild($doc->createElement('dcterms:modified', date('c', $object->changed)));

    // Create the docbook:article
    $docbook_article = $doc->createElement('docbook:article');
    $docbook_article->appendChild($doc->createElement('docbook:title', str_replace('&', '&amp;', $object->title)));
    $docbook_info = $doc->createElement('docbook:info');
    $docbook_publisher = $doc->createElement('docbook:publisher');
    $docbook_publisher->appendChild($doc->createElement('docbook:publishername', $source));
    $docbook_info->appendChild($docbook_publisher);
    $docbook_article->appendChild($docbook_info);

    // Create the section which holds the question and answer set.
    $docbook_section = $doc->createElement('docbook:section');
    $docbook_qandaset = $doc->createElement('docbook:qandaset');
    $docbook_qandadiv = $doc->createElement('docbook:qandadiv');
    $docbook_qandaentry = $doc->createElement('docbook:qandaentry');
    $docbook_question = $doc->createElement('docbook:question');
    $docbook_question->appendChild($doc->createElement('docbook:para', str_replace('&', '&amp;', $object->question_content)));
    $docbook_qandaentry->appendChild($docbook_question);
    $docbook_answer = $doc->createElement('docbook:answer');
    $docbook_answer->appendChild($doc->createElement('docbook:para', str_replace('&', '&amp;', $object->answer_content)));
    $docbook_qandaentry->appendChild($docbook_answer);
    $docbook_qandadiv->appendChild($docbook_qandaentry);
    $docbook_qandaset->appendChild($docbook_qandadiv);
    $docbook_section->appendChild($docbook_qandaset);
    $docbook_article->appendChild($docbook_section);

    // Add docbook:article to the container.
    $container->appendChild($docbook_article);
  }
  // Add the record container to the collection.
  $collection->appendChild($container);
}