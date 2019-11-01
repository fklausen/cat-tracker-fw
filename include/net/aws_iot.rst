.. _lib_aws_iot:

AWS IoT
#######

The Amazon Web Services Internet-of-Things library enables applications to connect to and exchange messages with the AWS IoT message broker. The library supports the following technologies:

* TLS secured MQTT transmission protocol
* Firmware-Over-The-Air (FOTA)

.. note::
   For documentation regarding the AWS FOTA library, see the :ref:`lib_aws_fota` documentation and the `aws_fota` sample. This library and the :ref:`lib_aws_fota` library share common steps related to `AWS IoT console`_ managing.

Setting up a connection to AWS IoT
**********************************

In order to setup a secure MQTTS connection to the AWS IoT message broker the following steps must be completed:

1. Creating a *thing* for your nRF9160 based board
2. Generate a CA certificate
3. Flash the certifcate to the nRF9160 on-board modem
4. Configure the library options

Creating a *thing* for your board nRF9160 based board
*****************************************************

1. Log on to the `AWS IoT console`_.
#. Go to **Secure** > **Policies** and select **Create a policy**.
#. Enter a name and define your policy.
   For testing purposes, you can use the following policy (switch to **Advanced mode** to copy and paste it):

   .. code-block:: javascript

      {
         "Version": "2012-10-17",
         "Statement": [
             {
               "Effect": "Allow",
               "Action": "iot:*",
               "Resource": "*"
             }
          ]
       }
#. Go to **Manage** > **Things** and select **Register a thing** or **Create** (depending on whether you already have a thing registered).
#. Select **Create a single thing**.
#. Enter a name.
   The default name used by the AWS IoT library is ``my-thing``.
#. Accept the defaults and continue to the next step.

Generate a CA certificate
*************************

1. Select **Create certificate** to generate new certificates.
#. Download the certificates
   You need the thing certificate (``*-certificate.pem.crt``), the private key (``*.private.pem.key``), and the root CA (choose the Amazon Root CA 1, ``AmazonRootCA1.pem``).
#. Click **Activate** to activate the certificates.
#. Click **Attach a policy** to continue to the next step.
#. Select the policy that you previously created and click **Register Thing**.

Flash the certificates to the nRF9160 on-board modem
****************************************************
1. Download the `nRF Connect` application which can be downloaded at https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-desktop/Download#infotabs
#. Flash the newest modem version to the nRF9160 on-board modem with the **programmer** application located in `nRF Connect`.
#. Flash the `at_client` sample to the nRF9160 using the **programmer**.
#. Launch the **LTE Link Monitor** application in *nRF Connect* and navigate to the **certificate manager** located in the top right corner.
#. Copy-paste the previously downloaded certificates from AWS IoT into the respective entries (``CA certificate``, ``Client certificate``, ``Private key``) and select a desired security tag and click **update certificates**.

.. note::
   The default security tag *16842753* is reserved for the :ref:`lib_nrf_cloud`

Configure library options
*************************

1. In the `AWS IoT console`_ navigate to **IoT core** -> **Manage** -> **things** and click on the newly created *thing*. Navigate to **interact** and find the **Rest API Endpoint**. Set the configurable option :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME` to this address.
2. Set the option :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` to the name of the *thing* created during the aformentioned steps.
3. Set the secuirty tag configuration :option:`CONFIG_AWS_IOT_SEC_TAG` to the to the previously chosen security tag.

Initializing
************

The module is initialized by calling :cpp:func:`aws_iot_init` function. If this API fails, the application must not use any APIs of the module.

Connecting
**********

.. note::
   The API requires that a configuration structure :c:type:`aws_iot_config` is declared in the application and passed in with the :cpp:func:`aws_iot_init` and :cpp:func:`aws_iot_connect` functions. This gives the application exposure to the MQTT socket used for the connection, so that it can be polled on in the application. It also enables the application to pass in a client id (*thingname*) at runtime.

After initiazation the :cpp:func:`aws_iot_connect` must be called to connect to the AWS IoT broker. If the API fails, the application must retry the connection. During an attempt to connect to the AWS Iot broker the library tries to establish a connection using a TLS handshake. This can take some time, usually in the domain of seconds. For the duration of the TLS handshake, the API blocks.

After a successful connection, the API subscribes to AWS IoT Shadow topics and application specific topics depending on the configuration of the library.

Polling on MQTT socket
**********************

After a successful return of :cpp:func:`aws_iot_connect` the MQTT socket must be polled on in addition to periodic calls to :cpp:func:`aws_iot_ping` to keep the connection to the AWS IoT broker alive, and periodic calls to :cpp:func:`aws_iot_input` in order to get data from the AWS IoT broker.

The code section below presents example code of how socket polling can be done in the main application after :cpp:func:`aws_iot_init` has been called.

   .. code-block:: c

      connect:
         err = aws_iot_connect(&config);
         if (err) {
            printk("aws_iot_connect failed: %d\n", err);
         }

         struct pollfd fds[] = {
            {
               .fd = config.socket,
               .events = POLLIN
            }
         };

         while (true) {
            err = poll(fds, ARRAY_SIZE(fds),
               K_SECONDS(CONFIG_MQTT_KEEPALIVE / 3));

            if (err < 0) {
               printk("poll() returned an error: %d\n", err);
               continue;
            }

            if (err == 0) {
               aws_iot_ping();
               continue;
            }

            if ((fds[0].revents & POLLIN) == POLLIN) {
               aws_iot_input();
            }

            if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
               printk("Socket error: POLLNVAL\n");
               printk("The AWS IoT socket was unexpectedly closed.\n");
               return;
            }

            if ((fds[0].revents & POLLHUP) == POLLHUP) {
               printk("Socket error: POLLHUP\n");
               printk("Connection was closed by the AWS IoT broker.\n");
               return;
            }

            if ((fds[0].revents & POLLERR) == POLLERR) {
               printk("Socket error: POLLERR\n");
               printk("AWS IoT broker connection was unexpectedly closed.\n");
               return;
            }
         }

         aws_iot_disconnect();
         goto connect;

Configuration
*************

In order to subscribe to *AWS shadow topics* the following options can be set:

- :option:`CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE`
- :option:`CONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE`

For subscription to non AWS specific topics the following option must be set: (specifying the number of additional topics that will be subscribed to)

- :option:`CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT`

.. note::
   The :cpp:func:`aws_iot_subscription_topics_add` function must be called with a list containing application topics, after :cpp:func:`aws_iot_init` and before :cpp:func:`aws_iot_connect`.

The following mandatory options must be set in order to connect to the AWS IoT broker, as specified in the `Configure library options` step of this guide:

- :option:`CONFIG_AWS_IOT_SEC_TAG`
- :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
- :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

Optionally a client id can be passed in by the application at run-time using by setting the :option:`CONFIG_AWS_IOT_CLIENT_ID_APP` option and setting the ``client_id`` entry in the :c:type:`aws_iot_config` structure passed in the :cpp:func:`aws_iot_init` function:

- :option:`CONFIG_AWS_IOT_CLIENT_ID_APP`

.. note::
   By default the library will use the static configurable option :option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC` for the client id.

.. note::
   The AWS IoT library is compatible with the generic *cloud_api* library, a generic API that supports interchangeable cloud backends, statically and at run-time.

API documentation
*****************

| Header file: :file:`include/net/aws_iot.h`
| Source files: :file:`subsys/net/lib/aws_iot/src/`

.. doxygengroup:: aws_iot
   :project: nrf
   :members:
