#include "../include/simulator.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
// We create a buffer to store all the msgs
struct buffer
{
  struct msg message;
  struct buffer *next;
};

bool sender_state = true;
struct pkt curr_packet;
int seq_num_A = 0;
int seq_num_B = 0;
int get_checksum(struct pkt *packet);
struct buffer *head = NULL;
struct buffer *tail = NULL;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message) struct msg message;
{
  // Creating new buffer using message
  printf("%s \n", message.data);
  struct buffer *new = (struct buffer *)malloc(sizeof(struct buffer));
  for (int i = 0; i < sizeof(new->message.data) / sizeof(new->message.data[0]); i++)
  {
    new->message.data[i] = '\0';
  }
  if (new == NULL)
  {
    printf("no enough memory\n");
  }
  else
  {
    new->next = NULL;
    strncpy(new->message.data, message.data, sizeof(message.data)/sizeof(message.data[0]));
    new->message.data[20]='\0';
    //  Adding new buffer in the existing buffer
    if (tail == NULL)
    {
      printf("Tail is null\n");
      tail = new;
      head = new;
    }
    else
    {
      tail->next = new;
      tail = new;
    }
  }

  // Retreive the first message in the buffer
  struct buffer *curr_buffer = head;
  printf(curr_buffer->message.data);
  printf("\n");
  if (curr_buffer == NULL)
  {
    printf("No msg to process\n");
    return;
  }
  if (!sender_state)
  {
    printf("sender not ready\n");
    return;
  }
  strncpy(curr_packet.payload, curr_buffer->message.data, 20);
  curr_packet.acknum = 1;
  curr_packet.seqnum = seq_num_A;
  curr_packet.checksum = get_checksum(&curr_packet);
  starttimer(0, 20.0);
  tolayer3(0, curr_packet);
  sender_state = false;

  // Setting head to next message
  head = head->next;
  if (head == NULL)
  {
    printf("Head is null\n");
    tail = NULL;
  }
  free(curr_buffer);
}

int get_checksum(struct pkt *pkt)
{
  int result = 0;
  if (pkt == NULL)
  {
    return result;
  }
  result = result + pkt->acknum;
  result = result + pkt->seqnum;
  int i = 0;
  while (i < 20)
  {
    result = result + (unsigned char)pkt->payload[i++];
  }
  return result;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet) struct pkt packet;
{
  if (packet.checksum != get_checksum(&packet))
  {
    printf("Wrong checksum at A\n");
    return;
  }
  if (packet.acknum != seq_num_A)
  {
    printf("Incorrect seq no at A\n");
    return;
  }
  seq_num_A = 1 - seq_num_A;
  sender_state = true;
  printf("Received ACK %d at A\n", packet.acknum);
  stoptimer(0);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  if (sender_state == true)
  {
    printf("Timer interrupt \n");
    tolayer3(0, curr_packet);
    starttimer(0, 20.0);
  }
  else{
    printf("Timer sender state false \n");
  }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet) struct pkt packet;
{
  if (packet.checksum != get_checksum(&packet))
  {
    printf("Wrong checksum at B\n");
    return;
  }

  if (packet.seqnum != seq_num_B)
  {
    printf("Incorrect seq number at B\n");
    return;
  }
  /* normal package, deliver data to layer5 */
  else
  {
    seq_num_B = 1 - seq_num_B;
    printf(packet.payload);
    printf("\n");
    tolayer5(1, packet.payload);
  }

  /* send back ack */
  packet.acknum = packet.seqnum;
  packet.checksum = get_checksum(&packet);
  tolayer3(1, packet);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
}
