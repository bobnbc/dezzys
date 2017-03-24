<?php

/**
 * @file
 * Provides admin forms and functions for commerce_payeezy.
 */

/**
 * Form callback for processing refunds.
 */
function commerce_payeezy_refund_form($form, &$form_state, $order, $transaction) {
  $form = array();
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  $form_state['order'] = $order;
  $form_state['transaction'] = $transaction;
  $form_state['payment_method'] = $payment_method;

   // Make sure the library is available.
  if (!commerce_payeezy_load_library()) {
    drupal_set_message(t('Cannot load the Payeezy PHP library'), 'error');
    return $form;
  }

  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Refund amount'),
    '#description' => t('Enter any amount to refund up to @txn_amount', array('@txn_amount' => commerce_currency_format($transaction->amount, $transaction->currency_code))),
    '#required' => TRUE,
    '#size' => 8,
  );

  $options = array(
    'requested_by_customer' => t('Requested by customer'),
    'duplicate' => t('Duplicate'),
    'fraudulent' => t('Fraduluent'),
  );

  $form['reason'] = array(
    '#type' => 'select',
    '#title' => t('Refund reason'),
    '#description' => t('Select the most appropriate reason for the refund.'),
    '#options' => $options,
  );

  $form['actions'] = array(
    '#type' => 'container',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Process refund'),
  );

  $form['actions']['cancel'] = array(
    '#type' => 'button',
    '#value' => t('Cancel'),
  );
  return $form;
}

/**
 * Validation callback for submitting refunds to Payeezy.
 */
function commerce_payeezy_refund_form_validate($form, &$form_state) {
  $transaction = $form_state['transaction'];
  $amount = commerce_currency_decimal_to_amount($form_state['values']['amount'], $transaction->currency_code);

  // Make sure the refund amount is valid and available.
  if ($amount <= 0  || !is_numeric($amount)) {
    form_set_error('amount', t('Please enter a valid return amount.'));
  }
}

/**
 * Submit callback for submitting refunds to Payeezy.
 */
function commerce_payeezy_refund_form_submit($form, &$form_state) {
  // Don't rely on form_state objects to be fresh.
  $order = commerce_order_load($form_state['order']->order_id);
  $transaction = commerce_payment_transaction_load($form_state['transaction']->transaction_id);
  $payment_method = $form_state['payment_method'];
  global $user;

  // Make sure the library is available.
  if (!commerce_payeezy_load_library()) {
    drupal_set_message(t('Cannot load the Payeezy PHP library'), 'error');
    return FALSE;
  }

  try {
    $client = new Payeezy_Client();
    $client->setApiKey($payment_method['settings']['api_key']);
    $client->setApiSecret($payment_method['settings']['api_secret']);
    $client->setMerchantToken($payment_method['settings']['merchant_token']);
    $client->setUrl($payment_method['settings']['transaction_url']);
    
    $purchase_response_in_json = reset($transaction->payload);
    $purchase_response = json_decode($purchase_response_in_json);
    if($purchase_response->correlation_id != $transaction->remote_id){
      throw new Exception("Couldn't retrieve the transaction with correlation_id: ".$purchase_response->correlation_id);
    }
    
    $refund_card_transaction = new Payeezy_CreditCard($client);
    $refund_response = $refund_card_transaction->refund(
      $purchase_response->transaction_id,
      array(
          "amount"=> commerce_currency_decimal_to_amount($form_state['values']['amount'], $transaction->currency_code),
          "transaction_tag" => $purchase_response->transaction_tag,
          "currency_code" => $transaction->currency_code,
          "merchant_ref" => $form_state['values']['reason'],
      )
    );
  }
  catch (Exception $e) {
    drupal_set_message(t('The transaction could not be refunded. The error was: @error', array('@error' => $e->getMessage())), 'error');
  }

  if (is_object($refund_response)) {
    // Create the new commerce payment transation and set appropriate values.
    $refund_transaction = commerce_payment_transaction_new($transaction->payment_method, $order->order_id);
    $refund_transaction->instance_id = $payment_method['instance_id'];
    $refund_transaction->payload[REQUEST_TIME] = json_encode($refund_response);
    if($refund_response->transaction_status == PAYEEZY_TRANSACTION_STATUS_SUCCESS){
      $refund_transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
      $refund_transaction->message = t('Refund issued.');
    }else{
      $refund_transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
      $refund_transaction->message = t('Refund processing failed.');
      // Inform the user that the refund was not successful.
      $mesasge = 'The payment transaction could not be refunded. Failure information, if provided is: @failure_code @failure_message';
      $message_variables = array(
          '@failure_code' => !empty($refund_response->transaction_status) ? $refund_response->transaction_status : NULL,
          '@failure_message' => !empty($refund_response->gateway_message) ? $refund_response->gateway_message : NULL,
      );
      drupal_set_message(t($message, $message_variables), 'error');
    }
    $refund_transaction->remote_id = $refund_response->correlation_id;

    // Save the amount as a negative integer.
    $refund_transaction->amount = $refund_response->amount * -1;
    $refund_transaction->currency_code = strtoupper($refund_response->currency);
    commerce_payment_transaction_save($refund_transaction);

    // Inform the user of the success and redirect them back to payments.
    drupal_set_message(t('Refund processed successfully'));
    $form_state['redirect'] = 'admin/commerce/orders/' . $order->order_id . '/payment';
    return;
  }
}
