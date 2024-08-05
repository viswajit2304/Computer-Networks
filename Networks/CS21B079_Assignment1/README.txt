Name : SV.Viswajit                Roll no: CS21B079
Name : G.Sai Pradhyumna           Roll no: CS21B026.

The drive link : https://drive.google.com/drive/folders/1L8kamdUrGfv9_mRI3HBQM79YbXb9dbuv?usp=sharing

Question-1:
Part-A:
We have drawn 4 graphs in total:
1) packets/sec vs time(seconds) for uplink
2) packets/sec vs time(seconds) for downlink
3) kilobytes/sec vs time(seconds) for uplink
4) kilobytes/sec vs time(seconds) for downlink
and each graph contains 5 sub graphs with different colors corresponding to the five resolutions of the video given, and the graph has x-axis with 60-seconds plotted in the granularity of 100 milliseconds.

The fraction of burst slots we obtained are
fraction of burst slots in 480p are 0.019966722129783693
fraction of burst slots in 720p are 0.07487520798668885
fraction of burst slots in 1080p are 0.15307820299500832
fraction of burst slots in 2k are 0.3610648918469218
fraction of burst slots in 4k are 0.8935108153078203
We even printed these values in the ipynb file.

part-B:
total time required for DNS query for deccan is 0.03372899999999995
total time required for DNS query for jagran is 0.04550900000000002
total time required for DNS query for mit is 0.05657800000000002
total time required for DNS query for sinu is 0.48806700000000003
total time required for DNS query for usach is 0.385584
We even printed these values in the ipynb file.

The insights that we observed are the following
1) The distance between the user and the server hosting the website can significantly impact the time it takes for a DNS query to complete.

Question-2:
We read the content that you provided in the assignment and done the question accordingly.

Question -3 :
For the sake of clarification we present the assumptions that we took while solving the given question,
I have taken the following cases as Invalid:

1) The opcode should not be more than 2
2) The Z-part of the bits in the packet :
   Reserved for future use. Must be zero in all queries and responses.
   So we checked the bits that corresponds to Z if any bit is non-zero , we assumed it as invalid
3) In the Domain Name the corresponding ASCII values should be finite.
   These are the characters corresponding to the ASCII values we assumed to be finite: 0-9|a-z|A-Z
4) Rcode - this 4 bit field is set as part of responses. The values have the following
 	   interpretation:
 	   Rcode = 0 => no error condition according to the text book
 	   So we assumed that if the Rcode in the given packet is not zero then we assumed that the given packet is invalid
5) We assumed that the QDCOUNT in the packet should be 0001 (in hexadecimal) to be valid
6) if it is query then the ANCOUNT should 0000 (in hexadecimal) and if it is reponse the ANCOUNT should be 0001 (in hexadecimal) for the packet to be valid.
7) if it is response packet then TYPE part in the packet should be 0001 (in hexadecimal) and CLASS part in the packet should 0001 (in hexadecimal) for the packet to be valid.

These are cases we assumed to be invalid and the rest of the question we did as instructed in the question.
