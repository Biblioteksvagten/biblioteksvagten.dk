<?php

/**
 * @file
 * Admin functions for Vopros statistics module.
 */

/**
 * Display statistics for a year.
 */
function vopros_statistics_year_view($year = NULL) {
  if (empty($year)) {
    $year = date('Y', REQUEST_TIME);
  }

  // Generate all of the weeks.
  $dates = array();
  foreach (range(1, 52) as $week_num) {
    $dates[$week_num] = array(
      ':start' => strtotime($year . 'W' . sprintf("%02d", $week_num) . '1'),
      ':end' => strtotime($year . 'W' . sprintf("%02d", $week_num + 1) . '1'),
    );

    if ($dates[$week_num][':end'] > REQUEST_TIME) {
      break;
    }
  }

  $header = array(t('Week number'), t('Total questions'));
  if (module_exists('vopros_editorial')) {
    $vocab = taxonomy_vocabulary_machine_name_load('vopros_editorial');
    $terms = taxonomy_term_load_multiple(FALSE, array('vid' => $vocab->vid));
    foreach ($terms as $term) {
      $header[] = $term->name;
    }
  }
  $rows = array();

  foreach ($dates as $week => $where) {
    $question_ids = db_query("SELECT question_id FROM {vopros_question} WHERE created >= :start AND created <= :end", $where)->fetchAllKeyed(0, 0);
    $row = array(
      $week,
      count($question_ids),
    );
    $question_ids[] = 0;
    if (!empty($terms)) {
      $editorials = db_query("SELECT COUNT(*), vopros_editorial_tid FROM field_data_vopros_editorial
                              WHERE entity_id IN (:question_ids) GROUP BY vopros_editorial_tid", array(':question_ids' => $question_ids))->fetchAllKeyed(1, 0);
      foreach ($terms as $tid => $term) {
        $row[] = $editorials[$tid];
      }
    }

    $rows[] = $row;
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Display count of created questions devided by the time it was created.
 */
function vopros_statistics_week_view($start = NULL, $end = NULL) {
  if (!$end) {
    $end = REQUEST_TIME;
  }
  if (!$start) {
    $start = REQUEST_TIME - 86400 * 7;
  }

  $result = db_query("SELECT DAYNAME(FROM_UNIXTIME(q.created)) AS day_name, COUNT(*) AS questions FROM {vopros_question} AS q
            WHERE created < :end
            AND created > :start
            GROUP BY day_name", array(':end' => $end, ':start' => $start))->fetchAllKeyed(0, 1);

  $header = array(t('Week day'), t('Total questions created'));
  $rows = array();
  // Create a unix timestamp for a random date that has the time 00:00:00.
  $weeks = array(
    'Monday' => t('Monday'),
    'Tuesday' => t('Tuesday'),
    'Wednesday' => t('Wednesday'),
    'Thursday' => t('Thursday'),
    'Friday' => t('Friday'),
    'Saturday' => t('Saturday'),
    'Sunday' => t('Sunday'),
  );

  // Create the total count.
  foreach ($weeks as $week_day => $label) {
    $rows[$week_day] = array($label, isset($result[$week_day]) ? $result[$week_day] : 0);
  }

  // Get statistics for each editorial.
  if (module_exists('vopros_editorial')) {
    $vocab = taxonomy_vocabulary_machine_name_load('vopros_editorial');
    $terms = taxonomy_term_load_multiple(FALSE, array('vid' => $vocab->vid));
    foreach ($terms as $tid => $term) {
      $header[] = $term->name;
      $result = db_query("SELECT DAYNAME(FROM_UNIXTIME(q.created)) AS day_name, COUNT(*) AS questions FROM {vopros_question} AS q
                                              INNER JOIN {field_data_vopros_editorial} AS f ON f.entity_id = q.question_id
                                              WHERE q.created < :end
                                              AND q.created > :start
                                              AND f.vopros_editorial_tid = :tid
                                              GROUP BY day_name", array(
                                                ':tid' => $tid,
                                                ':end' => $end,
                                                ':start' =>
                                                $start,
                                              ))->fetchAllKeyed(0, 1);
      foreach ($weeks as $week_day => $label) {
        $rows[$week_day][] = isset($result[$week_day]) ? $result[$week_day] : 0;
      }
    }
  }

  $build = array(
    'form' => drupal_get_form('vopros_statistics_week_form', $start, $end),
    'table' => array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    ),
  );
  return $build;
}

/**
 * Form for selecting the date range for the time statistics page.
 */
function vopros_statistics_week_form($form, &$form_state, $start, $end) {
  $form['start'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start for time range'),
    '#date_format' => 'd-m-Y',
    '#date_year_range' => '-10:+1',
    '#default_value' => date('Y-m-d', $start),
    '#required' => TRUE,
  );

  $form['end'] = array(
    '#type' => 'date_popup',
    '#title' => t('End for time range'),
    '#date_format' => 'd-m-Y',
    '#date_year_range' => '-10:+1',
    '#default_value' => date('Y-m-d', $end),
    '#required' => TRUE,
  );

  $form['get'] = array(
    '#type' => 'submit',
    '#value' => t('Get statistics'),
  );

  return $form;
}

/**
 * Submit handler for vopros_statistics_time_form()
 */
function vopros_statistics_week_form_submit($form, &$form_state) {
  $start = strtotime($form_state['values']['start']);
  $end = strtotime($form_state['values']['end']);
  $form_state['redirect'] = "admin/vopros/statistics/week/$start/$end";
}

/**
 * Display count of created questions devided by the time it was created.
 */
function vopros_statistics_time_view($start = NULL, $end = NULL) {
  if (!$end) {
    $end = REQUEST_TIME;
  }
  if (!$start) {
    $start = REQUEST_TIME - 86400 * 7;
  }

  $result = db_query("SELECT HOUR(FROM_UNIXTIME(q.created)) AS created_hour, COUNT(*) AS questions FROM {vopros_question} AS q
            WHERE created < :end
            AND created > :start
            GROUP BY created_hour
            ORDER BY created_hour", array(':end' => $end, ':start' => $start))->fetchAllKeyed(0, 1);

  $header = array(t('Time'), t('Questions created'));
  $rows = array();
  // Create a unix timestamp for a random date that has the time 00:00:00.
  $start_time = strtotime('2000-12-12 00:00:00');
  foreach (range(0, 23) as $time) {
    $count = isset($result[$time]) ? $result[$time] : 0;
    $rows[$time] = array(
      date('H:i:s', $start_time + $time * 3600) . ' - ' .
      date('H:i:s', $start_time + $time * 3600 + 3599), $count,
    );
  }

  if (module_exists('vopros_editorial')) {
    $vocab = taxonomy_vocabulary_machine_name_load('vopros_editorial');
    $terms = taxonomy_term_load_multiple(FALSE, array('vid' => $vocab->vid));
    foreach ($terms as $tid => $term) {
      $header[] = $term->name;
      $result = db_query("SELECT HOUR(FROM_UNIXTIME(q.created)) AS created_hour, COUNT(*) AS questions FROM {vopros_question} AS q
                          INNER JOIN {field_data_vopros_editorial} AS f ON f.entity_id = q.question_id
                          WHERE created < :end
                          AND created > :start
                          AND f.vopros_editorial_tid = :tid
                          GROUP BY created_hour
                          ORDER BY created_hour", array(
                            ':end' => $end,
                            ':start' => $start,
                            ':tid' => $tid,
                          ))->fetchAllKeyed(0, 1);
      foreach (range(0, 23) as $time) {
        $rows[$time][] = isset($result[$time]) ? $result[$time] : 0;
      }
    }
  }

  $build = array(
    'form' => drupal_get_form('vopros_statistics_time_form', $start, $end),
    'table' => array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    ),
  );
  return $build;
}

/**
 * Form for selecting the date range for the time statistics page.
 */
function vopros_statistics_time_form($form, &$form_state, $start, $end) {
  $form['start'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start for time range'),
    '#date_format' => 'd-m-Y',
    '#date_year_range' => '-10:+1',
    '#default_value' => date('Y-m-d', $start),
    '#required' => TRUE,
  );

  $form['end'] = array(
    '#type' => 'date_popup',
    '#title' => t('End for time range'),
    '#date_format' => 'd-m-Y',
    '#date_year_range' => '-10:+1',
    '#default_value' => date('Y-m-d', $end),
    '#required' => TRUE,
  );

  $form['get'] = array(
    '#type' => 'submit',
    '#value' => t('Get statistics'),
  );

  return $form;
}

/**
 * Submit handler for vopros_statistics_time_form()
 */
function vopros_statistics_time_form_submit($form, &$form_state) {
  $start = strtotime($form_state['values']['start']);
  $end = strtotime($form_state['values']['end']);
  $form_state['redirect'] = "admin/vopros/statistics/time/$start/$end";
}

/**
 * Display statistics based on Q and A service names.
 */
function vopros_statistics_client_channels_view($start = NULL, $end = NULL) {
  if (!$end) {
    $end = REQUEST_TIME;
  }
  if (!$start) {
    $start = REQUEST_TIME - 86400 * 7;
  }

  $clients = vopros_question_client_load_multiple();
  $channels = vopros_question_channels();

  // We are using the 0 tid for totals, cross editorial.
  $terms = array(0 => (object) array('name' => t('Total questions')));
  if (module_exists('vopros_editorial')) {
    $vocab = taxonomy_vocabulary_machine_name_load('vopros_editorial');
    $terms += taxonomy_term_load_multiple(FALSE, array('vid' => $vocab->vid));
  }

  // Get counts by client, channel and tid.
  $res = db_query("SELECT COUNT(*) AS count,
                      q.client_id AS client_id,
                      q.channel AS channel,
                      f.vopros_editorial_tid AS tid
                      FROM {vopros_question} AS q
                      LEFT JOIN {field_data_vopros_editorial} AS f
                      ON q.question_id = f.entity_id
                      WHERE created <= :end
                      AND created >= :start
                      GROUP BY client_id, channel, tid
                      ORDER BY client_id, channel, tid",
  array(':end' => $end, ':start' => $start));

  // Calculate totals and rearrange result for display.
  $totals = array();
  $column_totals = array();
  foreach ($res as $row) {
    // We handle the rows without a tid as tid 0. Means they're (mostly) not a
    // special case.
    $tid = $row->tid ? $row->tid : 0;
    $client_name = check_plain(isset($clients[$row->client_id]) ? $clients[$row->client_id]->label : '<unknown>');
    $channel_name = check_plain(isset($channels[$row->channel]) ? $channels[$row->channel]['name'] : '<unknown>');

    $totals[$client_name]['totals'][$tid] += $row->count;
    $totals[$client_name]['channels'][$channel_name][$tid] = $row->count;
    // Also add non-zero to the zero (the totals column) tid.
    if ($tid != 0) {
      $totals[$client_name]['totals'][0] += $row->count;
      $totals[$client_name]['channels'][$channel_name][0] += $row->count;
    }

    // Add everything to the column totals.
    $column_totals[$tid] += $row->count;
    // Again, add to the total column.
    if ($tid != 0) {
      $column_totals[0] += $row->count;
    }
  }

  $totals['<em>' . t('Total') . '</em>']['totals'] = $column_totals;

  $header = array(t('Client'));
  foreach ($terms as $tid => $term) {
    $header[] = $term->name;
  }

  $rows = array();
  foreach ($totals as $name => $data) {
    $row = array(
      $name,
    );
    foreach ($terms as $tid => $term) {
      $row[] = $data['totals'][$tid] + 0;
    }
    $rows[] = $row;
    foreach ($data['channels'] as $channel => $chan_data) {
      $row = array(
        ' - ' . $channel,
      );
      foreach ($terms as $tid => $term) {
        $row[] = $chan_data[$tid] + 0;
      }
      $rows[] = $row;

    }
  }

  $build = array(
    'form' => drupal_get_form('vopros_statistics_client_channels_form', $start, $end),
    'table' => array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    ),
  );
  return $build;
}

/**
 * Form for selecting the date range for the time statistics page.
 */
function vopros_statistics_client_channels_form($form, &$form_state, $start, $end) {
  $form['start'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start for time range'),
    '#date_format' => 'd-m-Y',
    '#date_year_range' => '-10:+1',
    '#default_value' => date('Y-m-d', $start),
    '#required' => TRUE,
  );

  $form['end'] = array(
    '#type' => 'date_popup',
    '#title' => t('End for time range'),
    '#date_format' => 'd-m-Y',
    '#date_year_range' => '-10:+1',
    '#default_value' => date('Y-m-d', $end),
    '#required' => TRUE,
  );

  $form['get'] = array(
    '#type' => 'submit',
    '#value' => t('Get statistics'),
  );

  return $form;
}

/**
 * Submit handler for vopros_statistics_client_channels_form()
 */
function vopros_statistics_client_channels_form_submit($form, &$form_state) {
  $start = strtotime($form_state['values']['start']);
  $end = strtotime($form_state['values']['end']);
  $form_state['redirect'] = "admin/vopros/statistics/client_channels/$start/$end";
}
