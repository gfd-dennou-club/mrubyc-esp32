import os
import glob
import re
import shutil

# 定数
#
pattern1 = 'BEGIN COMPONENTS 1'
pattern2 = 'END COMPONENTS 1'
pattern3 = 'BEGIN COMPONENTS 2'
pattern4 = 'END COMPONENTS 2'
num  = 0
num1 = 1
num2 = 1
num3 = 1
num4 = 1

# リンクを削除
#
print( "** remove symlinks **")
checks = [ 'mrblib/*.rb', 'main/*.c', 'main/*.h' ]
for dir in checks:
    files = glob.glob( dir )
    for file in files:
        if os.path.islink( file ):
            print( file )
            os.remove( file )
            
# main ファイルを戻す
#
f = open('main/main.c', 'r')
datalist = f.readlines()
f.close()

for tmp in datalist:
    data = tmp.rstrip('\n')       # 取り出した行の改行文字カット
    if re.search(pattern1, data): # main.c の特定行に合致したら
        num1 = num
    if re.search(pattern2, data): # main.c の特定行に合致したら
        num2 = num      
    if re.search(pattern3, data): # main.c の特定行に合致したら
        num3 = num
    if re.search(pattern4, data): # main.c の特定行に合致したら
        num4 = num        
    num += 1
    
# ファイルの書き込み
print( "\n** main/main.c is initialized **\n")
shutil.copy2('main/main.c', 'main/main.c.bk2') #バックアップ
f = open('main/main.c', 'w')
i = 0
for data in datalist:
    if not ( (i > num1 and i < (num2 - 1) ) or ( i > num3 and i < (num4 - 1)) ) :
        f.write( data )
    i += 1
f.close()    

