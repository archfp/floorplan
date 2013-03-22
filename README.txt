README for Floor Planner Source Code
Git has been setup on both PC and MAC
Ready to go!

Weekly Objective:
(01.21) Deliver the solution for all geoHints with simple case
(01.28) (Temporary) Design a recursive solution

Tasks:
1. Analyze how ParquetFP and BlockFiller can be of help.
   So far, we know that blockFiller can be used to fill in deadspaces from a FLP file. 
2. Review how the simple solution is implemented.
3. Re-test tofig.pl with amonhen.

Debugging Note:
You can google for the description about perl, fig2dev and ps2pdf. But all you need to do to get the output is:
1. Upload your tofig.pl and the .flp file to /data/<yourFolder> (eg. mine is hlu16) with FileZilla (or other FTP tools)
2. Login into the amonhen.ece.mcgill.ca with PuTTY
3. cd /data/<yourFolder>
4. perl tofig.pl Foo.flp | fig2dev -L ps | ps2pdf - Foo.pdf


Requirements:


