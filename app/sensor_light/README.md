# Sensor: Light
Using the CC2538DK ambient light sensor, this app will periodically
take a sample. When enough samples have been taken, the mean value will
be calculated. If associated with a coordinator, 802.15.4 messages
will be sent to the coordinator.