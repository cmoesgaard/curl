# MQTT in carl

## Usage

A plain "GET" subscribes to the topic and prints all published messages.
Doing a "POST" publishes the post data to the topic and exits.

Example subscribe:

    carl mqtt://host/home/bedroom/temp

Example publish:

    carl -d 75 mqtt://host/home/bedroom/dimmer

## What does carl deliver as a response to a subscribe

It outputs two bytes topic length (MSB | LSB), the topic followed by the
payload.

## Caveats

Remaining limitations:
 - No username support
 - Only QoS level 0 is implemented for publish
 - No way to set retain flag for publish
 - No username/password support
 - No TLS (mqtts) support
 - Naive EAGAIN handling won't handle split messages
