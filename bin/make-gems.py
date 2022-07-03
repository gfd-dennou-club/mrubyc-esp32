import os
import glob
import re
import shutil

# 初期化
#
insert1   = []
insert2   = []
cfiles    = []
rubyfiles = []

# 定数
#
exclude  = 'mrubyc'
pattern1 = 'BEGIN COMPONENTS 1'
pattern2 = 'BEGIN COMPONENTS 2'
dst_c    = 'main'
dst_ruby = 'mrblib'

# components 以下のファイルを検索 -> main.c へ書き込む内容に.
#
dirs = glob.glob('components/*')
for dir in dirs:
    if os.path.basename(dir) != "mrubyc":

        files = glob.glob(dir + '/mrblib/*rb')
        for file in files:
            path, ext = os.path.splitext( os.path.basename(file) )
            path2     = os.path.splitext( os.path.dirname(file) )
            rubyfiles+= [ [path2[0], path, file] ]
            
        files = glob.glob(dir + '/main/*c')
        for file in files:
            path, ext = os.path.splitext( os.path.basename(file) )
            path2     = os.path.splitext( os.path.dirname(file) )
            insert1  += ["#include \""+path+".h\""]
            insert2  += ["  "+path+"_gem_init(0);"]
            cfiles   += [ [path2[0], path, file] ]

# .rb ファイルのリンク
#
print("*** make links (.rb) ***")
for file in rubyfiles:
    src = "../"+file[2]
    dst = dst_ruby+"/"+file[1]+".rb"
    if not os.path.isfile( dst ):
        os.symlink( src, dst )
        print( src+" -> "+dst )

# .c, .h ファイルのリンク
#
print("\n*** make links (.c, .h) ***")
for file in cfiles:
    src = "../"+file[0]+"/"+file[1]+".c"
    dst = dst_c+"/"+file[1]+".c" 
    if not os.path.isfile( dst ):
        os.symlink( src, dst )
        print( src+" -> "+dst )

    src = "../"+file[0]+"/"+file[1]+".h"         
    dst = dst_c+"/"+file[1]+".h"
    if not os.path.isfile( dst ):
        os.symlink( src, dst )
        print( src+" -> "+dst )
        
# ファイルの全てを読み込み
#
f = open('main/main.c', 'r')
datalist = f.readlines()
f.close()

# ファイルの修正
#
print("\n*** add following lines in main/main.c ***")
datalist2 = []
for tmp in datalist:
    datalist2 += [tmp]       #行の保管
    data = tmp.rstrip('\n')  #取り出した行の改行文字カット
    
    if re.search(pattern1, data): # main.c の特定行に合致したら
        for file in insert1:      # 文字列の用意
            flag1 = 1
            for tmp2 in datalist: # 取り出した行
                if file.rstrip('\n') == tmp2.rstrip('\n'):
                    flag1 = 0
            if flag1 == 1:
                datalist2 += [file+"\n"]
                print( file )

    if re.search(pattern2, data):
        for file in insert2:
            flag = 1
            for tmp2 in datalist:
                if file.rstrip('\n') == tmp2.rstrip('\n'):
                    flag = 0
            if flag == 1:
                datalist2 += [file+"\n"]
                print( file )                        

# ファイルの書き込み
shutil.copy2('main/main.c', 'main/main.c.bk') #バックアップ
f = open('main/main.c', 'w')
f.writelines(datalist2)
f.close()
