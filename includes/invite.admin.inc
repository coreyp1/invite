<?php
// $Id: invite_admin.inc,v 1.3.2.3 2009/10/04 12:34:25 smk Exp $

/**
 * @file
 * Administration functions for the Invite module.
 */

invite_load_include('invite');

/**
 * Display module help.
 */
function invite_module_help() {
  $file = drupal_get_path('module', 'invite') .'/README.txt';
  if (file_exists($file)) {
    return _filter_autop(check_plain(file_get_contents($file)));
  }
}

/**
 * Display help text on user profile pages.
 */
function invite_user_help($path) {
  switch ($path) {
    case 'user/%/invites':
    case 'user/%/invites/accepted':
      $output = '<p>'. t("The invitations shown on this page have been used to join the site. Clicking on an e-mail address takes you to the user's profile page.");
      break;
    case 'user/%/invites/pending':
      $output = '<p>'. t("The invitations shown on this page haven't been accepted yet.");
      break;
    case 'user/%/invites/expired':
      $output = '<p>'. t('The invitations shown on this page have not been used to register on the site within the expiration period of @count days.', array('@count' => variable_get('invite_expiry', 30)));
      break;

    default:
      return;
  }
  $output .= ' '. t('The status <em>deleted</em> means the user account has been terminated.') .'</p>';
  if (!user_access('withdraw accepted invitations')) {
    $output .= '<p>'. t("At any time, you may withdraw either pending or expired invitations. Accepted invitations can't be withdrawn and count permanently toward your invitation allotment.") .'</p>';
  }
  return $output;
}

/**
 * Form builder; display invite settings form.
 *
 * @ingroup forms
 */
function invite_settings() {
  $roles = user_roles(FALSE, 'send invitations');
  if (count($roles) == 0) {
    drupal_set_message(t('Please enable the <em>send invitations</em> permission for at least one role. This can be done on the <a href="!permissions-url">Permissions page</a>.', array('!permissions-url' => url('admin/user/permissions'))));
  }
  $target_roles = user_roles(TRUE);

  // General settings.
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['general']['invite_target_role_default'] = array(
    '#type' => 'select',
    '#title' => t('Default target role'),
    '#default_value' => variable_get('invite_target_role_default', DRUPAL_AUTHENTICATED_RID),
    '#options' => $target_roles,
    '#description' => t('Choose the default role that invited users will be added to when they register. For example, <em>authenticated user</em>.'),
    '#required' => TRUE,
  );
  $form['general']['invite_expiry'] = array(
    '#type' => 'select',
    '#title' => t('Invitation expiry'),
    '#default_value' => variable_get('invite_expiry', 30),
    '#options' => drupal_map_assoc(array(1, 3, 7, 14, 30, 60)),
    '#description' => t('Set the expiry period for user invitations, in days.'),
    '#multiple' => FALSE,
    '#required' => TRUE,
  );

  // Role settings.
  $form['role'] = array(
    '#type' => 'fieldset',
    '#title' => t('Role settings'),
    '#description' => t('Note: Permission related settings can be found at the <a href="!permissions-url">Permissions page</a>.', array('!permissions-url' => url('admin/user/permissions'))),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  foreach ($roles as $rid => $role) {
    $form['role'][$rid] = array(
      '#type' => 'fieldset',
      '#title' => t('@role settings', array('@role' => drupal_ucfirst($role))),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['role'][$rid]['invite_target_role_'. $rid] = array(
      '#type' => 'select',
      '#title' => t('Target role'),
      '#default_value' => variable_get('invite_target_role_'. $rid, DRUPAL_AUTHENTICATED_RID),
      '#options' => $target_roles,
      '#description' => t('You may choose to add invited users to another role (in addition to the default role set in the general section) when they have been invited by a member of %role.', array('%role' => $role)),
      '#required' => TRUE,
    );
    $form['role'][$rid]['invite_maxnum_'. $rid] = array(
      '#type' => 'select',
      '#title' => t('Invitation limit'),
      '#default_value' => variable_get('invite_maxnum_'. $rid, INVITE_UNLIMITED),
      '#options' => array(5 => 5, 10 => 10, 20 => 20, 50 => 50, 100 => 100, 500 => 500, 1000 => 1000, INVITE_UNLIMITED => t('unlimited')),
      '#description' => t('Allows to limit the total number of invitations members of %role can send.', array('%role' => $role)),
      '#multiple' => FALSE,
      '#required' => TRUE,
    );
  }

  // E-mail settings.
  $form['email'] = array(
    '#type' => 'fieldset',
    '#title' => t('E-mail settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['email']['invite_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#default_value' => variable_get('invite_subject', t('[inviter-raw] has sent you an invite!')),
    '#size' => 30,
    '#maxlength' => 64,
    '#description' => t('Type the default subject of the invitation e-mail.') .' '. t('Use the syntax [token] if you want to insert a replacement pattern.'),
    '#required' => TRUE,
  );
  $form['email']['invite_subject_editable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Editable subject'),
    '#description' => t('Choose whether users should be able to customize the subject.'),
    '#default_value' => variable_get('invite_subject_editable', FALSE),
  );
  $form['email']['invite_default_mail_template'] = array(
    '#type' => 'textarea',
    '#title' => t('Mail template'),
    '#default_value' => invite_get_mail_template(),
    '#required' => TRUE,
    '#description' => t('Use the syntax [token] if you want to insert a replacement pattern.'),
  );
  $form['email']['token_help'] = array(
    '#title' => t('Replacement patterns'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['email']['token_help']['help'] = array(
    '#value' => theme('invite_token_help', array('user', 'profile', 'invite')),
  );
  $form['email']['invite_use_users_email'] = array(
    '#type' => 'radios',
    '#title' => t('<em>From</em> e-mail address'),
    '#description' => t('Choose which e-mail address will be in the From: header for the invitation mails sent; <em>site</em> or <em>inviter</em>. <em>Site</em> will use the default e-mail address of the site, whereas <em>inviter</em> will use the e-mail address of the user who is sending the invitation. Alternatively, you can set this value manually by clicking on <em>advanced settings</em> below.'),
    '#options' => array(t('site'), t('inviter')),
    '#default_value' => variable_get('invite_use_users_email', 0),
  );
  $form['email']['invite_use_users_email_replyto'] = array(
    '#type' => 'radios',
    '#title' => t('<em>Reply-To</em> e-mail address'),
    '#description' => t('Choose which e-mail address will be in the Reply-To: header for the invitation mails sent; <em>site</em> or <em>inviter</em>. <em>Site</em> will use the default e-mail address of the site, whereas <em>inviter</em> will use the e-mail address of the user who is sending the invitation. Alternatively, you can set this value manually by clicking on <em>advanced settings</em> below.'),
    '#options' => array(t('site'), t('inviter')),
    '#default_value' => variable_get('invite_use_users_email_replyto', 0),
  );
  $form['email']['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('<strong>Note:</strong> unless these fields are blank, they will override the radio button choices above.'),
  );
  $form['email']['advanced']['invite_manual_from'] = array(
    '#type' => 'textfield',
    '#title' => t('Manually override <em>From</em> e-mail address'),
    '#default_value' => variable_get('invite_manual_from', ''),
    '#description' => t('The e-mail address the invitation e-mail is sent from.'),
  );
  $form['email']['advanced']['invite_manual_reply_to'] = array(
    '#type' => 'textfield',
    '#title' => t('Manually override <em>Reply-To</em> e-mail address'),
    '#default_value' => variable_get('invite_manual_reply_to', ''),
    '#description' => t('The e-mail address you want recipients to reply to.'),
  );

  // Invite page customization.
  $form['custom'] = array(
    '#type' => 'fieldset',
    '#title' => t('Invite page customization'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['custom']['invite_page_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Invite page title'),
    '#default_value' => variable_get('invite_page_title', t('Invite a friend')),
    '#description' => t('The title of the page where users invite friends.'),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}

/**
 * Menu callback; render a list of all users that have invited someone.
 */
function invite_admin_overview() {
  $output = drupal_get_form('invite_admin_filter_form');

  // Generate custom SQL filter condition.
  $filter = $filter_args = $query = NULL;
  if (isset($_SESSION[INVITE_SESSION_ADMIN])) {
    $filter = " INNER JOIN {users} u2 ON u2.uid = i.invitee WHERE LOWER(u2.name) LIKE LOWER('%s%%')";
    $filter_args = $_SESSION[INVITE_SESSION_ADMIN];
    $query = 'filter='. $filter_args;
    unset($_SESSION[INVITE_SESSION_ADMIN]);
  }

  $header = array(
    array('data' => t('Username'), 'field' => 'u1.name', 'sort' => 'asc'),
    array('data' => t('Total'), 'field' => 'invites'),
    t('Successful'),
    t('Pending'),
    t('Expired'),
    t('Remaining'),
    t('Operations'),
  );

  $sql = "SELECT i.uid, u1.name, COUNT(*) AS invites, u1.data FROM {invite} i INNER JOIN {users} u1 ON u1.uid = i.uid". $filter ." GROUP BY i.uid, u1.name, u1.data";
  $sql .= tablesort_sql($header);
  $count_sql  = "SELECT COUNT(DISTINCT i.uid) FROM {invite} i". $filter;
  $result = pager_query($sql, 50, 0, $count_sql, $filter_args);

  $rows = array();
  while ($row = db_fetch_object($result)) {
    $cells = array();
    $cells[] = theme('username', $row);
    $cells[] = $row->invites;
    $cells[] = invite_count($row->uid, 'accepted');
    $cells[] = invite_count($row->uid, 'pending');
    $cells[] = invite_count($row->uid, 'expired');
    $remaining = invite_get_remaining($row);
    $cells[] = ($remaining == INVITE_UNLIMITED) ? '&infin;' : $remaining;
    $cells[] = l(t('details'), "admin/user/invite/details/$row->uid", array('query' => $query));
    $rows[] = $cells;
  }

  $output .= theme('table', $header, $rows, array('id' => 'invite'));
  if (!$rows) {
    $output .= t('No inviters found.');
  }
  else {
    $output .= theme('pager', array(), 50, 0);
  }

  return $output;
}

/**
 * Form builder; creates the user filter form for the admin overview page.
 *
 * @ingroup forms
 * @see invite_admin_filter_form_submit()
 */
function invite_admin_filter_form() {
  $form['filter'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $form['filter']['filter'] = array(
    '#type' => 'textfield',
    '#title' => t('Find an invited user'),
    '#default_value' => isset($_SESSION[INVITE_SESSION_ADMIN]) ? $_SESSION[INVITE_SESSION_ADMIN] : '',
    '#autocomplete_path' => 'user/autocomplete',
    '#size' => 20,
  );
  $form['filter']['op'] = array(
    '#type' => 'submit',
    '#value' => t('Search'),
  );
  return $form;
}

/**
 * Forms API callback; stores the selected user filter in a session variable.
 *
 * @see invite_admin_filter_form()
 */
function invite_admin_filter_form_submit($form, &$form_state) {
  if ($form_state['values']['filter']) {
    $_SESSION[INVITE_SESSION_ADMIN] = $form_state['values']['filter'];
  }
}

/**
 * Return a list of invites by a user.
 *
 * @param $account
 *   A user object.
 */
function invite_admin_details($account) {
  $output = '';
  $now = time();

  // Generate custom SQL sort conditions.
  // FIXME Use SQL standard.
  $status_sort = '';
  if (isset($_GET['order']) && $_GET['order'] == t('Status')) {
    $sort = db_escape_string($_GET['sort']);
    if ($_GET['sort'] == 'asc') {
      $status_sort = "IF(i.joined != 0, 0, 1) $sort, IF(i.expiry < $now, 0, 1) $sort, i.canceled";
    }
    else {
      $status_sort = "i.canceled $sort, IF(i.expiry < $now, 0, 1) $sort, IF(i.joined != 0, 0, 1)";
    }
  }

  $header = array(
    array('data' => t('E-mail'), 'field' => 'i.email', 'sort' => 'asc'),
    array('data' => t('Username'), 'field' => 'IF(u.name IS NULL, 0, 1) DESC, u.name'),
    array('data' => t('Created'), 'field' => 'i.created'),
    array('data' => t('Expires'), 'field' => 'i.expiry'),
    array('data' => t('Status'), 'field' => $status_sort),
  );

  // Generate custom SQL filter condition.
  $filter = $filter_args = NULL;
  if (isset($_GET['filter']) && $_GET['filter'] != '') {
    $filter = " AND LOWER(u.name) LIKE '%s%%'";
    $filter_args = $_GET['filter'];
    $output .= drupal_get_form('invite_admin_details_filter_form', $account->uid, $_GET['filter']);
  }

  $sql = "SELECT i.email, i.invitee AS uid, u.name, i.created, i.expiry, i.joined, i.canceled FROM {invite} i LEFT JOIN {users} u ON u.uid = i.invitee AND u.uid <> 0 WHERE i.uid = %d". $filter;
  $sql .= tablesort_sql($header);
  $result = pager_query($sql, 50, 0, NULL, $account->uid, $filter_args);

  $rows = array();
  while ($row = db_fetch_object($result)) {
    $cells = array();
    $cells[] = check_plain($row->email);
    $cells[] = $row->joined ? theme('username', $row) : '';
    $cells[] = format_date($row->created, 'custom', 'Y-m-d');
    $cells[] = format_date($row->expiry, 'custom', 'Y-m-d');
    $cells[] = $row->canceled ? t('Withdrawn') : ($row->joined ? t('Joined') : ($row->expiry < $now ? t('Expired') : t('Pending')));
    $rows[] = $cells;
  }

  $output .= theme('table', $header, $rows, array('id' => 'invite'));
  if (!$rows) {
    $output .= t('No invitees found.');
  }
  else {
    $output .= theme('pager', array(), 50, 0);
  }

  return $output;
}

/**
 * Form builder; creates the active user filter form.
 *
 * @ingroup forms
 */
function invite_admin_details_filter_form($uid, $filter) {
  $form['#action'] = "admin/user/invite/details/$uid";
  $form['filter'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $form['filter']['filter'] = array(
    '#type' => 'item',
    '#title' => t('Active filter'),
    '#value' => check_plain($filter),
  );
  $form['filter']['op'] = array(
    '#type' => 'button',
    '#value' => t('Clear'),
    '#prefix' => ' ',
  );
  return $form;
}

/**
 * Menu callback; generate an overview of sent invitations for a user.
 *
 * @param $account
 *   A user object.
 * @param $page
 *   Which invites to list: accepted, pending, or expired.
 */
function invite_user_overview($account, $page = 'accepted') {
  $rows = array();
  $time = time();
  $profile_access = user_access('access user profiles');
  $allow_delete = user_access('withdraw accepted invitations');

  switch ($page) {
    case 'accepted':
    default:
      $sql = "SELECT i.*, u.uid AS account FROM {invite} i LEFT JOIN {users} u ON u.uid = i.invitee AND u.uid <> 0 WHERE i.uid = %d AND i.joined <> 0 AND canceled = 0 ORDER BY u.uid DESC";
      break;
    case 'pending':
      $sql = "SELECT * FROM {invite} WHERE uid = %d AND joined = 0 AND canceled = 0 AND expiry >= %d ORDER BY expiry DESC";
      break;
    case 'expired':
      $sql = "SELECT * FROM {invite} WHERE uid = %d AND joined = 0 AND canceled = 0 AND expiry < %d ORDER BY expiry DESC";
      break;
  }

  $result = pager_query($sql, 50, 0, NULL, $account->uid, $time);

  while ($invite = db_fetch_object($result)) {
    $row = array();
    switch ($page) {
      case 'accepted':
      default:
        $account_exists = !is_null($invite->account);

        if ($profile_access && $account_exists) {
          $row[] = l($invite->email, 'user/'. $invite->invitee, array('title' => t('View user profile.')));
        }
        else {
          $row[] = check_plain($invite->email);
        }
        $row[] = $account_exists ? t('Accepted') : t('Deleted');
        $row[] = $allow_delete ? l(t('withdraw'), "invite/withdraw/$page/$invite->code") : '';
        $row[] = '';
        break;

      case 'pending':
      case 'expired':
        $expired = ($invite->expiry < $time);

        $row[] = check_plain($invite->email);
        $row[] = $expired ? t('Expired') : t('Pending');
        $row[] = l(t('withdraw'), "invite/withdraw/$page/$invite->code");
        $row[] = $expired ? l(t('resend'), "invite/resend/$invite->code") : '';
        break;
    }
    $rows[] = $row;
  }

  return theme('invite_user_overview', $rows);
}

/**
 * Theme function; display the invite overview table.
 *
 * @param $items
 *   An array of table rows.
 *
 * @ingroup themeable
 */
function theme_invite_user_overview($items) {
  if (count($items) > 0) {
    $headers = array(t('E-mail'), t('Status'), array('data' => t('Operations'), 'colspan' => 2));
    $output  = theme('table', $headers, $items, array('id' => 'invites'));
    $output .= theme('pager', NULL, 50, 0);
  }
  else {
    $output = t('No invitations available.');
  }

  return $output;
}

