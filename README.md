 */Binary-translator$ mkdir build*

 */Binary-translator$ cd build*

 */Binary-translator/build$ cmake ../CMakeLists.txt*

 */Binary-translator/build$ make*

 */Binary-translator/build$ cd ..*

 -----Optional---------------------------------

Configure execute.sh file

 -----Optional-end-----------------------------

 */Binary-translator/$ bash execute.sh*

 OR

 */Binary-translator/$ bash -x execute.sh*

 to see execution details

Output is written in $EXAMPLE_FOLDER/RawFile and is ready to be flashed via avrdude.
