/*
		scan complete for global variable
*/

#include "bbs.h"
#include "read.h"

extern time_t login_start_time;
static int yank_flag=0;

#define favbrd_list_t (*(getSession()->favbrd_list_count))
extern int do_select(struct _select_def* conf,struct fileheader *fileinfo,void* extraarg);
static int check_newpost(struct newpostdata *ptr);

void EGroup(cmd)
char *cmd;
{
    char buf[STRLEN];
    const char *boardprefix;

    sprintf(buf, "EGROUP%c", *cmd);
    boardprefix = sysconf_str(buf);
    choose_board(DEFINE(getCurrentUser(), DEF_NEWPOST) ? 1 : 0, boardprefix,0,0);
}

void ENewGroup(cmd)
char *cmd;
{
	choose_board(DEFINE(getCurrentUser(),DEF_NEWPOST) ? 1 : 0, NULL, -2, 0);
}

static int clear_all_board_read_flag_func(struct boardheader *bh,void* arg)
{
#ifdef HAVE_BRC_CONTROL
    if (brc_initial(getCurrentUser()->userid, bh->filename, getSession()) != 0)
        brc_clear(getSession());
#endif
    return 0;
}

int clear_all_board_read_flag()
{
    char ans[4];
    struct boardheader* save_board;
    int bid;

    getdata(t_lines - 1, 0, "清除所有的未读标记么(Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {

        bid=currboardent;
        save_board=currboard;
        apply_boards(clear_all_board_read_flag_func,NULL);
        currboard=save_board;
        currboardent=bid;
    }
    return 0;
}

void Boards()
{
    choose_board(0, NULL,0,0);
}

void New()
{
    choose_board(1, NULL,0,0);
}

int unread_position(dirfile, ptr)
char *dirfile;
struct newpostdata *ptr;
{
#ifdef HAVE_BRC_CONTROL
    struct fileheader fh;
    int id;
    int fd, offset, step, num;

    num = ptr->total + 1;
    if (ptr->unread && (fd = open(dirfile, O_RDWR)) > 0) {
        if (!brc_initial(getCurrentUser()->userid, ptr->name, getSession())) {
            num = 1;
        } else {
            offset = (int) ((char *) &(fh.id) - (char *) &(fh));
            num = ptr->total - 1;
            step = 4;
            while (num > 0) {
                lseek(fd, offset + num * sizeof(fh), SEEK_SET);
                if (read(fd, &id, sizeof(unsigned int)) <= 0 || !brc_unread(id, getSession()))
                    break;
                num -= step;
                if (step < 32)
                    step += step / 2;
            }
            if (num < 0)
                num = 0;
            while (num < ptr->total) {
                lseek(fd, offset + num * sizeof(fh), SEEK_SET);
                if (read(fd, &id, sizeof(unsigned int)) <= 0 || brc_unread(id, getSession()))
                    break;
                num++;
            }
        }
        close(fd);
    }
    if (num < 0)
        num = 0;
    return num;
#else
    return 0;
#endif
}

/* Select the fav path
seperated by pig2532@newsmth */
static int fav_select_path(const char *brdname, int *i)
{
    struct boardheader bh;
    int k;

    load_favboard(0,1, getSession());

    *i=getboardnum(brdname, &bh);
    if (*i<=0)
        return SHOW_REFRESH;
    if(favbrd_list_t < 2) {
        SetFav(0, getSession());
        if (IsFavBoard(*i - 1, getSession())) {
            move(2, 0); 
            clrtoeol();
            prints("已存在该讨论区.");
            clrtoeol();
            pressreturn();
            return SHOW_REFRESH;
        }
        move(2,0);
        if (askyn("加入个人定制区？",0)!=1)
            return SHOW_REFRESH;
    }
    else {
        struct _select_item *sel;
        char root[18]="定制区主目录";
        clear();
        move(3, 3);
        prints("请选择加入到定制区哪个目录");
        sel = (struct _select_item *) malloc(sizeof(struct _select_item) * (favbrd_list_t+1));
        sel[0].x = 3;
        sel[0].y = 6;
        sel[0].hotkey = '0';
        sel[0].type = SIT_SELECT;
        sel[0].data = root;
        for(k=1;k<favbrd_list_t;k++){
                sel[k].x = 3;
                sel[k].y = 6+k;
                sel[k].hotkey = '0'+k;
                sel[k].type = SIT_SELECT;
                sel[k].data = getSession()->favbrd_list[k].title;
        }
        sel[k].x = -1;
        sel[k].y = -1;
        sel[k].hotkey = -1;
        sel[k].type = 0;
        sel[k].data = NULL;
        k = simple_select_loop(sel, SIF_NUMBERKEY | SIF_SINGLE | SIF_ESCQUIT, 0, 6, NULL) - 1;
        free(sel);
        if(k>=0&&k<favbrd_list_t) {
            SetFav(k, getSession());
        }
        else
            return SHOW_REFRESH;
    }
    return DONOTHING;
}

/* Add board to fav
seperated by pig2532@newsmth
parameters:
    i: board id
return:
    0: success
    1: fav board exists
    2: error board
*/
static int fav_add_board(int i, int favmode)
{
    if (i > 0 && !IsFavBoard(i - 1, getSession())) {
        addFavBoard(i - 1, getSession());
	save_favboard(favmode, getSession());
        return 0;
    } else if (IsFavBoard(i - 1, getSession())) {
        return 1;
    } else {
        return 2;
    }
}

int show_boardinfo(const char *bname)
{
    const struct boardheader *bp = getbcache(bname);
    char ch;
    int ret, bid;

	if(bp==NULL)
		return 0;

	clear();

	move(2,0);
	prints("\033[1;33m版面名称\033[m: %s %s\n\n", bp->filename, bp->title+1);
	prints("\033[1;33m版面版主\033[m: %s \n", bp->BM);
#ifdef NEWSMTH
	prints("\033[1;31m版面web地址\033[m: http://%s.board.newsmth.net/ \n", bp->filename);
#endif
	prints("\033[1;33m版面关键字\033[m: %s \n\n", bp->des);
    prints("\033[1;31m%s\033[m记文章数 \033[1;31m%s\033[m统计十大\n", 
        (bp->flag & BOARD_JUNK) ? "不" : "", (bp->flag & BOARD_POSTSTAT) ? "不" : "");
    prints("\033[1;31m%s\033[m可向外转信 \033[1;31m%s\033[m可粘贴附件 \033[1;31m%s\033[m可re文\n\n", 
			(bp->flag & BOARD_OUTFLAG) ? "" : "不",
			(bp->flag & BOARD_ATTACH) ? "" : "不",
			(bp->flag & BOARD_NOREPLY) ? "不" : "");

    if(HAS_PERM(getCurrentUser(),PERM_SYSOP)){
        move(15,0);
        prints("\033[1;33m邀请限制\033[m: %s%s\n",
            ((bp->flag&BOARD_CLUB_HIDE)&&(bp->flag&(BOARD_CLUB_READ|BOARD_CLUB_WRITE)))?"隐藏":"",
            ((bp->flag&BOARD_CLUB_READ)&&(bp->flag&BOARD_CLUB_WRITE))?"读写限制俱乐部":
            ((bp->flag&BOARD_CLUB_READ)?"读限制俱乐部":
            ((bp->flag&BOARD_CLUB_WRITE)?"写限制俱乐部":"无")));
        prints("\033[1;33m权限限制\033[m: %s <%s>\n",
            (bp->level&~PERM_POSTMASK)?((bp->level&PERM_POSTMASK)?"发表限制":"读取限制"):"无限制",
            gen_permstr(bp->level,genbuf));
#ifdef HAVE_CUSTOM_USER_TITLE
        prints("\033[1;33m身份限制\033[m: %s <%d>",bp->title_level?get_user_title(bp->title_level):"无限制",
            (unsigned char)bp->title_level);
#endif
    }
    move(t_lines - 1, 0);
    prints("\033[m\033[44m        添加到个人定制区[\033[1;32ma\033[m\033[44m]");
    clrtoeol();
    resetcolor();
    ch = igetkey();
    switch(toupper(ch)) {
    case 'A':
        if(fav_select_path(bname, &bid) == DONOTHING)
        {
            ret = fav_add_board(bid, 1);
            switch(ret) {
            case 0:
                move(2, 0);
                prints("已经将 %s 讨论区添加到个人定制区.", bname);
                clrtoeol();
                pressreturn();
                return 2;
            case 1:
                move(2, 0);
                prints("已存在该讨论区.");
                clrtoeol();
                pressreturn();
		break;
            case 2:
                move(2, 0);
                prints("不正确的讨论区.");
                clrtoeol();
                pressreturn();
		break;
            }
        }
        break;
    }
    return 1;
}

/* etnlegend, 2005.10.16, 查询版主更新 */
int query_bm_core(const char *userid,int limited){
    struct userec *user;
    const struct boardheader *bh;
    char buf[16];
    int line,count,n;
    clear();
    move(0,0);
    prints("\033[1;32m[查询任职版面]\033[m");
    if(!userid||!*userid){
        move(1,0);
        usercomplete("查询用户: ",buf);
        move(1,0);
        clrtobot();
    }
    else
        sprintf(buf,"%s",userid);
    if(!*buf||!getuser(buf,&user)){
        move(1,0);
        prints("\033[1;31m取消查询或非法用户...\033[1;37m<Enter>\033[m");
        WAIT_RETURN;
        clear();
        return FULLUPDATE;
    }
    sprintf(buf,"%s",user->userid);
    move(1,0);
    prints("\033[1;37m用户 \033[1;33m%s\033[m %s\033[1;37m并出现于下列版面的版主列表中\033[m",
        buf,HAS_PERM(user,PERM_BOARDS)?"\033[1;37m有版主权限":"\033[1;31m无版主权限");
    for(line=3,count=0,n=0;n<get_boardcount();n++){
        if(!(bh=getboard(n+1))||!*(bh->filename))
            continue;
        if(limited&&!check_read_perm(getCurrentUser(),bh))
            continue;
        if(chk_BM_instr(bh->BM,buf)){
            count++;
            if(!(line<t_lines-3)){
                int key;
                move(line+1,0);
                prints("\033[1;32m按 \033[1;33m<Enter>/<Space>\033[1;32m 继续查询或者 \033[1;33m<Esc>/<Q>\033[1;32m 结束查询: \033[m");
                do{
                    key=igetch();
                }
                while(key!=13&&key!=32&&key!=27&&key!=113&&key!=81);
                if(key==13||key==32){
                    line=2;
                    move(line++,0);
                    clrtobot();
                }
                else
                    break;
            }
            move(line++,0);
            sprintf(genbuf,"\033[1;37m[%02d] %s %-32s %-32s\033[m",count,
                (line%2)?"\033[1;33m":"\033[1;36m",bh->filename,bh->title+13);
            prints("%s",genbuf);
        }
    }
    move(line+1,0);
    clrtoeol();
    prints("\033[1;32m查询%s\033[1;32m完成: 查询到 \033[1;33m%d\033[1;32m 个任职版面...\033[1;37m<Enter>\033[m",
        (n<get_boardcount())?"\033[1;33m未":"已",count-((n<get_boardcount())?1:0));
    WAIT_RETURN;
    clear();
    return FULLUPDATE;
}

int query_bm(void){
    return query_bm_core(NULL,!HAS_PERM(getCurrentUser(),PERM_SYSOP));
}
/* END - etnlegend, 2005.10.16, 查询版主更新 */

static int check_newpost(struct newpostdata *ptr)
{
    struct BoardStatus *bptr;

    if (ptr->dir)
        return 0;

    ptr->total = ptr->unread = 0;

    bptr = getbstatus(ptr->pos+1);
    if (bptr == NULL)
        return 0;
    ptr->total = bptr->total;
    ptr->currentusers = bptr->currentusers;

#ifdef HAVE_BRC_CONTROL
    if (!brc_initial(getCurrentUser()->userid, ptr->name, getSession())) {
        ptr->unread = 1;
    } else {
        if (brc_unread(bptr->lastpost, getSession())) {
            ptr->unread = 1;
        }
    }
#endif
    return 1;
}


enum board_mode {
    BOARD_BOARD,
    BOARD_BOARDALL,
    BOARD_FAV
};

struct favboard_proc_arg {
    struct newpostdata *nbrd;
    int favmode;
    int newflag;
    enum board_mode yank_flag;
    int father; /*保存父结点，如果是收藏夹，是fav_father,
    如果是版面目录，是group编号*/
    bool reloaddata;
    int select_group; //选择了一个目录
	/* stiger: select_group:  
	   在 select_loop退出，返回SHOW_SELECT的时候
	   select_group 0: 不是s进入目录
	                1: s进入版面目录
					<0, s进入allboard目录, i=0-select_group是 favboard_list[i-1]
					                       或者i=-1-select_group 是 favboard_list[i]
	*/

    const char* boardprefix;
    /*用于search_board得时候缓存*/
    int loop_mode;
    int find;
    char bname[BOARDNAMELEN + 1];
    int bname_len;
    const char** namelist;
};

static int search_board(int *num, struct _select_def *conf, int key)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    int n, ch, tmpn = false;
    extern bool ingetdata;
    ingetdata = true;

    *num=0;
    if (arg->find == true) {
        bzero(arg->bname, BOARDNAMELEN);
        arg->find = false;
        arg->bname_len = 0;
    }
    if (arg->namelist==NULL) {
    	arg->namelist=(const char**)malloc(MAXBOARD*sizeof(char*));
        memset(arg->namelist, 0, MAXBOARD*sizeof(char*));
    	conf->get_data(conf,-1,-1);
    }
    while (1) {
        move(t_lines - 1, 0);
        clrtoeol();
        prints("请输入要找寻的 board 名称：%s", arg->bname);
        if (key == -1)
            ch = igetkey();
        else {
            ch = key;
            key = -1;
        }

        if (ch == KEY_REFRESH)
            break;
        if (isprint2(ch)) {
            arg->bname[arg->bname_len++] = ch;
            for (n = 0; n < conf->item_count; n++) {
                if (!(arg->namelist[n])) continue;
                if ((!strncasecmp(arg->namelist[n], arg->bname, arg->bname_len))&&*num==0) {
                    tmpn = true;
                    *num = n;
                }
                if (!strcmp(arg->namelist[n], arg->bname)) {
        			ingetdata = false;
                    return 1 /*找到类似的版，画面重画 */ ;
				}
            }
            if (tmpn) {
        		ingetdata = false;
                return 1;
			}
            if (arg->find == false) {
                arg->bname[--arg->bname_len] = '\0';
            }
            continue;
        } else if (ch == Ctrl('H') || ch == KEY_LEFT || ch == KEY_DEL || ch == '\177') {
            arg->bname_len--;
            if (arg->bname_len< 0) {
                arg->find = true;
                break;
            } else {
                arg->bname[arg->bname_len]=0;
                continue;
            }
        } else if (ch == '\t') {
            arg->find = true;
            break;
        } else if (Ctrl('Z') == ch) {
            r_lastmsg();        /* Leeward 98.07.30 support msgX */
            break;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
            arg->find = true;
            break;
        }
        bell();
    }
    if (arg->find) {
        move(t_lines - 1, 0);
        clrtoeol();
        ingetdata = false;
        return 2 /*结束了 */ ;
    }
    ingetdata = false;
    return 1;
}

static int fav_show(struct _select_def *conf, int pos)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;
    char buf[LENGTH_SCREEN_LINE];
    bool isdir=false;

    ptr = &arg->nbrd[pos-(conf->page_pos)];
    if ((ptr->dir == 1)&&arg->favmode) {        /* added by bad 2002.8.3*/
        if (ptr->tag < 0)
            prints("       ");
        else if (!arg->newflag){
                isdir=true;
		if(arg->favmode == 1)
            	    prints(" %4d  ＋  <目录>  ", pos);
		else
            	    prints(" %4d  ＋  %-17s  ", pos ,ptr->BM);
		}else{
		    if(arg->favmode == 1)
            	        prints(" %4d  ＋  <目录>  ", ptr->total);
	            else
            	        prints(" %4d  ＋  %-17s  ", ptr->total, ptr->BM);
		}
    } else {
	if ((ptr->dir==1)||(ptr->flag&BOARD_GROUP)) {
            prints(" %4d  ＋ ", ptr->total);
            isdir=true;
	} else {
        if (!arg->newflag){
			check_newpost(ptr);
            prints(" %4d  %s ", pos, ptr->unread?"◆" : "◇");     /*zap标志 */
		}
        else if (ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) {
            /*
             * ptr->total = ptr->unread = 0;
             * prints( "    -    -" ); 
             */
            /*
             * Leeward: 97.12.15: extended display 
             */
            check_newpost(ptr);
            prints(" %4d%s%s ", ptr->total, ptr->total > 9999 ? " " : "  ", ptr->unread ? "◆" : "◇"); /*是否未读 */
        } else {
            if (ptr->total == -1) {
                /*refresh();*/
                check_newpost(ptr);
            }
            prints(" %4d%s%s ", ptr->total, ptr->total > 9999 ? " " : "  ", ptr->unread ? "◆" : "◇"); /*是否未读 */
        }
      }
    }
    /*
     * Leeward 98.03.28 Displaying whether a board is READONLY or not 
     */
    if (ptr->dir == 2)
        sprintf(buf, "%s(%d)", ptr->title, ptr->total);
    else if ((ptr->dir >= 1)&&arg->favmode)
        sprintf(buf, "%s", ptr->title); /* added by bad 2002.8.3*/
    else if (true == checkreadonly(ptr->name))
        sprintf(buf, "[只读] %s", ptr->title + 8);
    else
        sprintf(buf, "%s", ptr->title + 1);

    if ((ptr->dir >= 1)&&arg->favmode)          /* added by bad 2002.8.3*/
        prints("%-50s", buf);
    else {
          char flag[20];
          char f;
          char tmpBM[BM_LEN + 1];

          strncpy(tmpBM, ptr->BM, BM_LEN);
          tmpBM[BM_LEN] = 0;
  
          if ((ptr->flag & BOARD_CLUB_READ) && (ptr->flag & BOARD_CLUB_WRITE))
                 f='A';
          else if (ptr->flag & BOARD_CLUB_READ)
                f = 'c';
          else if (ptr->flag & BOARD_CLUB_WRITE)
                f = 'p';
          else
                f = ' ';
          if (ptr->flag & BOARD_CLUB_HIDE) {
	            sprintf(flag,"\x1b[1;31m%c\x1b[m",f);
	       } else if (f!=' ') {
	           sprintf(flag,"\x1b[1;33m%c\x1b[m",f);
          } else sprintf(flag,"%c",f);
#ifdef BOARD_SHOW_ONLINE
          if (!isdir)
          prints("%c%-16s%s%s%-32s %4d %-12s", ((ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) ? '*' : ' '), 
                ptr->name, (ptr->flag & BOARD_VOTEFLAG) ? "\033[31;1mV\033[m" : " ", flag, buf, 
                ptr->currentusers>9999?9999:ptr->currentusers,ptr->BM[0] <= ' ' ? "诚征版主中" : strtok(tmpBM, " "));
          else 
          prints("%c%-16s%s%s%-32s      %-12s", ((ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) ? '*' : ' '), 
                ptr->name, (ptr->flag & BOARD_VOTEFLAG) ? "\033[31;1mV\033[m" : " ", flag, buf, 
                ptr->BM[0] <= ' ' ? "诚征版主中" : strtok(tmpBM, " ")); /*第一个版主 */
#else
          prints("%c%-16s %s%s%-36s %-12s", ((ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) ? '*' : ' '), ptr->name,
                (ptr->flag & BOARD_VOTEFLAG) ? "\033[31;1mV\033[m" : " ", flag, buf, 
                 ptr->BM[0] <= ' ' ? "诚征版主中" : strtok(tmpBM, " ")); /*第一个版主 */
#endif
    }
    prints("\n");
    return SHOW_CONTINUE;
}

static int fav_prekey(struct _select_def *conf, int *command)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;

    if (arg->loop_mode) {
        int tmp, num;
/* search_board有问题，目前我们只有一页的数据，只能search
一页*/
        tmp = search_board(&num, conf, *command);
        if (tmp == 1) {
            conf->new_pos = num + 1;
            arg->loop_mode = 1;
            move(t_lines - 1, 0);
            clrtoeol();
            prints("请输入要找寻的 board 名称：%s", arg->bname);
            return SHOW_SELCHANGE;
        } else {
            arg->find = true;
            arg->bname_len = 0;
            arg->loop_mode = 0;
            conf->new_pos = num + 1;
            return SHOW_REFRESH;
        }
        return SHOW_REFRESH;
    }

    if (!arg->loop_mode) {
        int y,x;
        getyx(&y, &x);
        update_endline();
        move(y, x);
    }
    ptr = &arg->nbrd[conf->pos - conf->page_pos];
    switch (*command) {
    case 'e':
    case 'q':
        *command = KEY_LEFT;
        break;
    case 'p':
    case 'k':
        *command = KEY_UP;
        break;
    case ' ':
    case 'N':
        *command = KEY_PGDN;
        break;
    case 'n':
    case 'j':
        *command = KEY_DOWN;
        break;
	case 'r':
		*command = KEY_RIGHT;
		break;
    };
    return SHOW_CONTINUE;
}

static int fav_gotonextnew(struct _select_def *conf)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    int tmp, num,page_pos=conf->page_pos;

        /*搜寻下一个未读的版面*/
    if (arg->newflag) {
      num = tmp = conf->pos;
      while(num<=conf->item_count) {
          while ((num <= page_pos+conf->item_per_page-1)&&num<=conf->item_count) {
              struct newpostdata *ptr;
  
              ptr = &arg->nbrd[num - page_pos];
              if ((ptr->total == -1) && (ptr->dir == 0))
                  check_newpost(ptr);
              if (ptr->unread)
                  break;
                  num++;
          }
          if ((num <= page_pos+conf->item_per_page-1)&&num<=conf->item_count) {
              conf->pos = num;
              conf->page_pos=page_pos;
  	     return SHOW_DIRCHANGE;
  	 }
          page_pos+=conf->item_per_page;
          num=page_pos;
          (*conf->get_data)(conf, page_pos, conf->item_per_page);
      }
      if (page_pos!=conf->page_pos)
        (*conf->get_data)(conf, conf->page_pos, conf->item_per_page);
    }
    return SHOW_REFRESH;
}

static int fav_onselect(struct _select_def *conf)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    char buf[STRLEN];
    struct newpostdata *ptr;

    ptr = &arg->nbrd[conf->pos - conf->page_pos];

    if (arg->select_group) return SHOW_SELECT; //select a group
    arg->select_group=0;
	if (ptr->dir==1 && ptr->pos==-1 && ptr->flag==0xffffffff && ptr->tag==-1)
		return SHOW_CONTINUE;
    if ((ptr->dir == 1)||((arg->favmode)&&(ptr->flag&BOARD_GROUP))) {        /* added by bad 2002.8.3*/
        return SHOW_SELECT;
    } else {
        struct boardheader bh;
        int tmp;

        if (getboardnum(ptr->name, &bh) != 0 && check_read_perm(getCurrentUser(), &bh)) {
            int bid;
	    int returnmode;
            bid = getbnum(ptr->name);

            currboardent=bid;
            currboard=(struct boardheader*)getboard(bid);

#ifdef HAVE_BRC_CONTROL
            brc_initial(getCurrentUser()->userid, ptr->name, getSession());
#endif
            memcpy(currBM, ptr->BM, BM_LEN - 1);
            if (DEFINE(getCurrentUser(), DEF_FIRSTNEW)&&(getPos(DIR_MODE_NORMAL,currboard->filename,currboard)==0)) {
                setbdir(DIR_MODE_NORMAL, buf, currboard->filename);
                tmp = unread_position(buf, ptr);
                savePos(DIR_MODE_NORMAL,currboard->filename,tmp+1,currboard);
            }
            while (1) {
                returnmode=Read();
                if (returnmode==CHANGEMODE) { //select another board
                    if (currboard->flag&BOARD_GROUP) {
			arg->select_group=1;
                        return SHOW_SELECT;
                    }
                } else break;
            }
            (*conf->get_data)(conf, conf->page_pos, conf->item_per_page);
            modify_user_mode(SELECT);
            if (arg->newflag) { /* 如果是readnew的话，则跳到下一个未读版 */
                return fav_gotonextnew(conf);
            }
        }
        return SHOW_REFRESH;
    }
    return SHOW_CONTINUE;
}

static int fav_key(struct _select_def *conf, int command)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;

    ptr = &arg->nbrd[conf->pos - conf->page_pos];
    switch (command) {
    case Ctrl('Z'):
        r_lastmsg();            /* Leeward 98.07.30 support msgX */
        break;
    case 'X':                  /* Leeward 98.03.28 Set a board READONLY */
        {
            if (!HAS_PERM(getCurrentUser(), PERM_SYSOP) && !HAS_PERM(getCurrentUser(), PERM_OBOARDS))
                break;
            if (!strcmp(ptr->name, "syssecurity")
                || !strcmp(ptr->name, "Filter")
                || !strcmp(ptr->name, "junk")
                || !strcmp(ptr->name, "deleted"))
                break;          /* Leeward 98.04.01 */
            if (ptr->dir)
                break;

            if (strlen(ptr->name)) {
                board_setreadonly(ptr->name, 1);

                /*
                 * Bigman 2000.12.11:系统记录 
                 */
                sprintf(genbuf, "只读讨论区 %s ", ptr->name);
                securityreport(genbuf, NULL, NULL);
                sprintf(genbuf, " readonly board %s", ptr->name);
                newbbslog(BBSLOG_USER, "%s", genbuf);

                return SHOW_REFRESHSELECT;
            }
            break;
        }
    case 'Y':                  /* Leeward 98.03.28 Set a board READABLE */
        {
            if (!HAS_PERM(getCurrentUser(), PERM_SYSOP) && !HAS_PERM(getCurrentUser(), PERM_OBOARDS))
                break;
            if (ptr->dir)
                break;

            board_setreadonly(ptr->name, 0);

            /*
             * Bigman 2000.12.11:系统记录 
             */
            sprintf(genbuf, "解开只读讨论区 %s ", ptr->name);
            securityreport(genbuf, NULL, NULL);
            sprintf(genbuf, " readable board %s", ptr->name);
            newbbslog(BBSLOG_USER, "%s", genbuf);

            return SHOW_REFRESHSELECT;
        }
        break;
    case 'L':
    case 'l':                  /* Luzi 1997.10.31 */
        if (uinfo.mode != LOOKMSGS) {
            show_allmsgs();
            return SHOW_REFRESH;
        }
        break;
    case 'W':
    case 'w':                  /* Luzi 1997.10.31 */
        if (!HAS_PERM(getCurrentUser(), PERM_PAGE))
            break;
        s_msg();
        return SHOW_REFRESH;
    case 'u':                  /*Haohmaru.99.11.29 */
        {
            int oldmode = uinfo.mode;

            clear();
            modify_user_mode(QUERY);
            t_query(NULL);
            modify_user_mode(oldmode);
            return SHOW_REFRESH;
        }
	case 'U':		/* pig2532 2005.12.10 */
		board_query();
		if (BOARD_FAV == arg->yank_flag)
			return SHOW_DIRCHANGE;
		else
			return SHOW_REFRESH;
	/*add by stiger */
    case 'H':
	{
		read_hot_info();
    	return SHOW_REFRESH;
	}
	/* add end */
    case '!':
        Goodbye();
        return SHOW_REFRESH;
    case 'O':
    case 'o':                  /* Luzi 1997.10.31 */
#ifdef NINE_BUILD
    case 'C':
    case 'c':
#endif
        {                       /* Leeward 98.10.26 fix a bug by saving old mode */
            int savemode;

            savemode = uinfo.mode;

            if (!HAS_PERM(getCurrentUser(), PERM_BASIC))
                break;
            t_friends();
            modify_user_mode(savemode);
            return SHOW_REFRESH;
        }
#ifdef NINE_BUILD
    case 'F':
    case 'f':
#else
    case 'C':
    case 'c':                  /*阅读模式 */
#endif
        if (arg->newflag == 1)
            arg->newflag = 0;
        else
            arg->newflag = 1;
        return SHOW_REFRESH;
    case 'h':
        show_help("help/boardreadhelp");
        return SHOW_REFRESH;
	case Ctrl('A'):
        if (ptr->dir)
            break;
        {
            int oldmode;
            oldmode = uinfo.mode;
            modify_user_mode(QUERYBOARD);
            show_boardinfo(ptr->name);
            modify_user_mode(oldmode);
        }
        return SHOW_REFRESH;
    case '/':                  /*搜索board */
        {
            int tmp, num;

            tmp = search_board(&num, conf , -1);
            if (tmp == 1) {
                conf->new_pos = num + 1;
                arg->loop_mode = 1;
                move(t_lines - 1, 0);
                clrtoeol();
                prints("请输入要找寻的 board 名称：%s", arg->bname);
                return SHOW_SELCHANGE;
            } else {
                arg->find = true;
                arg->bname_len = 0;
                arg->loop_mode = 0;
                conf->new_pos = num + 1;
                return SHOW_REFRESH;
            }
            return SHOW_REFRESH;
        }
    case 'S':

		if (!(getCurrentUser()->flags & BRDSORT_FLAG)){
			getCurrentUser()->flags |= BRDSORT_FLAG;
			getCurrentUser()->flags &= ~BRDSORT1_FLAG;
		}else if(getCurrentUser()->flags & BRDSORT1_FLAG){
			getCurrentUser()->flags &= ~BRDSORT_FLAG;
			getCurrentUser()->flags &= ~BRDSORT1_FLAG;
		}else{
			getCurrentUser()->flags |= BRDSORT1_FLAG;
		}
        /*排序方式 */
        return SHOW_DIRCHANGE;
    case 's':                  /* sort/unsort -mfchen */
	{
		int tmp=1;

        if (do_select(0, NULL, &tmp) == CHANGEMODE) {
			if (tmp>0){
				arg->select_group = 0 - tmp;
				return SHOW_SELECT;
			}
            if (!(currboard->flag&BOARD_GROUP)) {
                while (1) {
                    int returnmode;
                    returnmode=Read();
                    if (returnmode==CHANGEMODE) { //select another board
                        if (currboard->flag&BOARD_GROUP) {
                            arg->select_group=1;
                            return SHOW_SELECT;
                        }
                    } else break;
                }
            }
            else {
				arg->select_group=1;
                return SHOW_SELECT;
            }
        }
        modify_user_mode(arg->newflag ? READNEW : READBRD);
        return SHOW_REFRESH;
	}
	case Ctrl('E'):
		{
			int newlevel;
			int oldlevel;
			if(arg->yank_flag != BOARD_FAV)
				return SHOW_CONTINUE;
			if(arg->favmode != 2 && arg->favmode!=3 )
				return SHOW_CONTINUE;
			if(!HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_CONTINUE;
			if(! ptr->dir)
				return SHOW_CONTINUE;
			if(ptr->tag <= 0 || ptr->tag >= favbrd_list_t)
				return SHOW_CONTINUE;
			clear();
			oldlevel = getSession()->favbrd_list[ptr->tag].level;
            newlevel = setperms(oldlevel, 0, "权限", NUMPERMS, showperminfo, NULL);
			if( newlevel != oldlevel){
				getSession()->favbrd_list[ptr->tag].level = newlevel;
				save_favboard(2, getSession());
				return SHOW_DIRCHANGE;
			}else
				return SHOW_REFRESH;

		}
    case Ctrl('O'):
    case 'a':
        {
            char bname[STRLEN];
            int i = 0, ret;
            extern int in_do_sendmsg;
            extern int super_select_board(char*);

            if (BOARD_FAV == arg->yank_flag) {

			if ((arg->favmode == 2 || arg->favmode == 3) && !HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_REFRESH;


            if (getSession()->favbrd_list[getSession()->favnow].bnum >= MAXBOARDPERDIR) {
                move(2, 0);
                clrtoeol();
                prints("个人热门版数已经达上限(%d)！", FAVBOARDNUM);
                pressreturn();
                return SHOW_REFRESH;
            }
                move(0, 0);
                clrtoeol();
                prints("输入讨论区英文名 (大小写皆可，按空白键或Tab键自动搜寻): ");
                clrtoeol();

                make_blist(0);
                in_do_sendmsg=1;
                if(namecomplete(NULL,bname)=='#')
                    super_select_board(bname);
                in_do_sendmsg=0;

                CreateNameList();   /*  free list memory. */
                if (*bname)
                    i = getbnum(bname);
                if (i==0)
                    return SHOW_REFRESH;
            } else {
                ret = fav_select_path(ptr->name, &i);
                if(ret != DONOTHING)
                {
                    return(ret);
                }
            }
	    if (BOARD_FAV == arg->yank_flag)
	    {
	        ret = fav_add_board(i, arg->favmode);
	    }
	    else
	    {
	        ret = fav_add_board(i, 1);
	    }
            switch(ret) {
            case 0:
                arg->reloaddata=true;
                if (BOARD_FAV == arg->yank_flag)
                    return SHOW_DIRCHANGE;
                else
                    return SHOW_REFRESH;
                break;
            case 1:
                move(2, 0);
                prints("已存在该讨论区.");
                clrtoeol();
                pressreturn();
                return SHOW_REFRESH;
            case 2:
                move(2, 0);
                prints("不正确的讨论区.");
                clrtoeol();
                pressreturn();
                return SHOW_REFRESH;
            }
        }
        break;
    case 'A':                  /* added by bad 2002.8.3*/
        if (BOARD_FAV == arg->yank_flag) {
            char bname[STRLEN];

			if ((arg->favmode == 2 || arg->favmode == 3) && !HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_REFRESH;


            if (favbrd_list_t >= FAVBOARDNUM) {
                move(2, 0);
                clrtoeol();
                prints("个人热门版数已经达上限(%d)！", FAVBOARDNUM);
                pressreturn();
                return SHOW_REFRESH;
            }
            move(0, 0);
            clrtoeol();
            getdata(0, 0, "输入讨论区目录名: ", bname, 41, DOECHO, NULL, true);
            if (bname[0]) {
                addFavBoardDir(bname, getSession());
                save_favboard(arg->favmode, getSession());
                arg->reloaddata=true;
                return SHOW_DIRCHANGE;
            }
            return SHOW_REFRESH;	/* add by pig2532 on 2005.12.3 */
        }
        break;
    case 't':
        if (BOARD_FAV == arg->yank_flag) {
            char bname[20];

			if ((arg->favmode == 2 || arg->favmode == 3) && !HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_REFRESH;

			if (arg->favmode == 1)
				return SHOW_REFRESH;

            if (ptr->dir == 1 && ptr->tag >= 0) {
                move(0, 0);
                clrtoeol();
                getdata(0, 0, "输入讨论区英文目录名(用于s选择): ", bname, 19, DOECHO, NULL, true);
                if (bname[0]) {
                    changeFavBoardDirEname(ptr->tag, bname, getSession());
                    save_favboard(arg->favmode, getSession());
                    return SHOW_REFRESH;
                }
            }
        }
        break;
    case 'T':                  /* added by bad 2002.8.3*/
        if (BOARD_FAV == arg->yank_flag) {
            char bname[STRLEN];

			if ((arg->favmode == 2 || arg->favmode == 3) && !HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_REFRESH;


            if (ptr->dir == 1 && ptr->tag >= 0) {
                move(0, 0);
                clrtoeol();
                getdata(0, 0, "输入讨论区目录名: ", bname, 41, DOECHO, NULL, true);
                if (bname[0]) {
                    changeFavBoardDir(ptr->tag, bname, getSession());
                    save_favboard(arg->favmode, getSession());
                    return SHOW_REFRESH;
                }
            }
        }
        break;
    case 'm':
        if (arg->yank_flag == BOARD_FAV) {

			if ((arg->favmode == 2 || arg->favmode == 3) && !HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_REFRESH;

            if (getCurrentUser()->flags & BRDSORT_FLAG) {
                move(0, 0);
                prints("排序模式下不能移动，请用'S'键切换!");
                pressreturn();
            } else {
                    int p, q;
                    char ans[5];

					if( ptr->dir )
						p=ptr->pos;
					else
						p=ptr->tag;

                    move(0, 0);
                    clrtoeol();
                    getdata(0, 0, "请输入移动到的位置:", ans, 4, DOECHO, NULL, true);
                    q = atoi(ans) - 1;
                    if (q < 0 || q >= conf->item_count) {
                        move(2, 0);
                        clrtoeol();
                        prints("非法的移动位置！");
                        pressreturn();
                    } else {
                        arg->father=MoveFavBoard(p, q, getSession());
                        save_favboard(arg->favmode, getSession());
                        arg->reloaddata=true;
                        return SHOW_DIRCHANGE;
                    }
            }
            return SHOW_REFRESH;
        }
        break;
    case 'd':
        if (BOARD_FAV == arg->yank_flag) {

			if ((arg->favmode == 2 || arg->favmode == 3) && !HAS_PERM(getCurrentUser(),PERM_SYSOP))
				return SHOW_REFRESH;

            if (ptr->tag >= 0){
                move(0, 0);
                clrtoeol();
                if( ! askyn("确认删除吗？", 0) )
					return SHOW_REFRESH;
				if(ptr->dir)
					DelFavBoardDir(ptr->pos,getSession()->favnow, getSession());
				else
                	DelFavBoard(ptr->pos, getSession());
                save_favboard(arg->favmode, getSession());
                arg->father=getSession()->favnow;
                arg->reloaddata=true;
                return SHOW_DIRCHANGE;
            }
            return SHOW_REFRESH;
        }
    case 'y':
        if (arg->yank_flag < BOARD_FAV) {
                                /*--- Modified 4 FavBoard 2000-09-11	---*/
            arg->yank_flag = 1 - arg->yank_flag;
            arg->reloaddata=true;
            return SHOW_DIRCHANGE;
        }
        break;
    case 'z':                  /* Zap */
        if (arg->yank_flag < BOARD_FAV) {
                                /*--- Modified 4 FavBoard 2000-09-11	---*/
            if (HAS_PERM(getCurrentUser(), PERM_BASIC) && !(ptr->flag & BOARD_NOZAPFLAG)) {
                ptr->zap = !ptr->zap;
                ptr->total = -1;
                getSession()->zapbuf[ptr->pos] = (ptr->zap ? 0 : login_start_time);
                getSession()->zapbuf_changed = 1;
                return SHOW_REFRESHSELECT;
            }
        }
        break;
    case 'v':                  /*Haohmaru.2000.4.26 */
        if(!strcmp(getCurrentUser()->userid, "guest") || !HAS_PERM(getCurrentUser(), PERM_READMAIL)) return SHOW_CONTINUE;
        clear();
		if (HAS_MAILBOX_PROP(&uinfo, MBP_MAILBOXSHORTCUT))
			MailProc();
		else
        	m_read();
        return SHOW_REFRESH;
    }
    return SHOW_CONTINUE;
}

static int fav_refresh(struct _select_def *conf)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;
	int sort;

    clear();
    ptr = &arg->nbrd[conf->pos - conf->page_pos];
    if (DEFINE(getCurrentUser(), DEF_HIGHCOLOR)) {
        if (arg->yank_flag == BOARD_FAV){
			if(arg->favmode == 2 || arg->favmode == 3){
            	docmdtitle("[讨论区列表]",
                       "  \033[m主选单[\x1b[1;32m←\x1b[m,\x1b[1;32me\x1b[m] 阅读[\x1b[1;32m→\x1b[m,\x1b[1;32mr\x1b[m] 选择[\x1b[1;32m↑\x1b[m,\x1b[1;32m↓\x1b[m] 排序[\x1b[1;32mS\x1b[m] 求助[\x1b[1;32mh\x1b[m]");
			}else{
            	docmdtitle("[个人定制区]",
                       "  \033[m主选单[\x1b[1;32m←\x1b[m,\x1b[1;32me\x1b[m] 阅读[\x1b[1;32m→\x1b[m,\x1b[1;32mr\x1b[m] 选择[\x1b[1;32m↑\x1b[m,\x1b[1;32m↓\x1b[m] 添加[\x1b[1;32ma\x1b[m,\x1b[1;32mA\x1b[m] 移动[\x1b[1;32mm\x1b[m] 删除[\x1b[1;32md\x1b[m] 排序[\x1b[1;32mS\x1b[m] 求助[\x1b[1;32mh\x1b[m]");
			}
		}else
            docmdtitle("[讨论区列表]",
                       "  \033[m主选单[\x1b[1;32m←\x1b[m,\x1b[1;32me\x1b[m] 阅读[\x1b[1;32m→\x1b[m,\x1b[1;32mr\x1b[m] 选择[\x1b[1;32m↑\x1b[m,\x1b[1;32m↓\x1b[m] 列出[\x1b[1;32my\x1b[m] 排序[\x1b[1;32mS\x1b[m] 搜寻[\x1b[1;32m/\x1b[m] 切换[\x1b[1;32mc\x1b[m] 求助[\x1b[1;32mh\x1b[m]");
    } else {
        if (arg->yank_flag == BOARD_FAV){
			if(arg->favmode == 2 || arg->favmode == 3){
            	docmdtitle("[讨论区列表]", "  \033[m主选单[←,e] 阅读[→,r] 选择[↑,↓] 排序[S] 求助[h]");
			}else{
            	docmdtitle("[个人定制区]", "  \033[m主选单[←,e] 阅读[→,r] 选择[↑,↓] 添加[a,A] 移动[m] 删除[d] 排序[S] 求助[h]");
			}
		}else
            docmdtitle("[讨论区列表]", "  \033[m主选单[←,e] 阅读[→,r] 选择[↑,↓] 列出[y] 排序[S] 搜寻[/] 切换[c] 求助[h]");
    }
    move(2, 0);
    setfcolor(WHITE, DEFINE(getCurrentUser(), DEF_HIGHCOLOR));
    setbcolor(BLUE);
    clrtoeol();
	sort = (getCurrentUser()->flags & BRDSORT_FLAG) ? ( (getCurrentUser()->flags&BRDSORT1_FLAG)+1):0;
#ifdef BOARD_SHOW_ONLINE
    prints("  %s %s讨论区名称%s       V 类别 转信  %-20s %s在线%s 版  主     ", arg->newflag ? "全部 未读" : "编号 未读", (sort==1)?"\033[36m":"", (sort==1)?"\033[44;37m":"", "中  文  叙  述", (sort & BRDSORT1_FLAG)?"\033[36m":"", (sort & BRDSORT1_FLAG)?"\033[44;37m":"");
#else
    prints("  %s 讨论区名称        V 类别 转信  %-24s 版  主     ", arg->newflag ? "全部 未读" : "编号 未读", "中  文  叙  述");
#endif
    resetcolor();
    if (!arg->loop_mode)
        update_endline();
    else {
            move(t_lines - 1, 0);
            clrtoeol();
            prints("请输入要找寻的 board 名称：%s", arg->bname);
    }
    return SHOW_CONTINUE;
}

static int fav_getdata(struct _select_def *conf, int pos, int len)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
	int sort;

    if (arg->reloaddata) {
    	arg->reloaddata=false;
    	if (arg->namelist) 	{
    		free(arg->namelist);
    		arg->namelist=NULL;
    	}
    }

	sort = (getCurrentUser()->flags & BRDSORT_FLAG) ? ( (getCurrentUser()->flags&BRDSORT1_FLAG)+1):0;
    if (pos==-1) 
        fav_loaddata(NULL, arg->father,1, conf->item_count,sort,arg->namelist, getSession());
    else{
		if((arg->favmode==2 || arg->favmode==3)){
    		if(!strcmp(getSession()->favbrd_list[arg->father].ename, "HotBoards")) sort=BRDSORT1_FLAG+1;
		}
        conf->item_count = fav_loaddata(arg->nbrd, arg->father,pos, len,sort,NULL, getSession());
	}
    return SHOW_CONTINUE;
}

static int boards_getdata(struct _select_def *conf, int pos, int len)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
	int sort;

    if (arg->reloaddata) {
    	arg->reloaddata=false;
    	if (arg->namelist) 	{
    		free(arg->namelist);
    		arg->namelist=NULL;
    	}
    }
	sort = (getCurrentUser()->flags & BRDSORT_FLAG) ? ( (getCurrentUser()->flags&BRDSORT1_FLAG)+1):0;
    if (pos==-1) 
         load_boards(NULL, arg->boardprefix,arg->father,1, conf->item_count,sort,arg->yank_flag,arg->namelist, getSession());
    else
         conf->item_count = load_boards(arg->nbrd, arg->boardprefix,arg->father,pos, len,sort,arg->yank_flag,NULL, getSession());
    return SHOW_CONTINUE;
}
int choose_board(int newflag, const char *boardprefix,int group,int favmode)
{
/* 选择 版， readnew或readboard */
    struct _select_def favboard_conf;
    struct favboard_proc_arg arg;
    POINT *pts;
    int i;
    struct newpostdata *nbrd;
    int favlevel = 0;           /*当前层数 */
    int favlist[FAVBOARDNUM];   /* 保存当前的group 信息和收藏夹信息*/
    int sellist[FAVBOARDNUM];   /*保存目录递归信息 */
    int oldmode;
    int changelevel=-1; /*保存在那一级的目录转换了mode,就是从收藏夹进入了版面目录*/
    int selectlevel=-1; /*保存在哪一级进入了s目录*/
	int oldfavmode=favmode;
	int lastloadfavmode = favmode;

    oldmode = uinfo.mode;
    modify_user_mode(SELECT);
#ifdef NEW_HELP
	if(favmode)
		helpmode = HELP_GOODBOARD;
	else
		helpmode = HELP_BOARD;
#endif
    clear();
    //TODO: 窗口大小动态改变的情况？这里有bug
    pts = (POINT *) malloc(sizeof(POINT) * BBS_PAGESIZE);

    for (i = 0; i < BBS_PAGESIZE; i++) {
        pts[i].x = 1;
        pts[i].y = i + 3;
    };

    nbrd = (struct newpostdata *) malloc(sizeof(*nbrd) * BBS_PAGESIZE);
    arg.nbrd = nbrd;
    sellist[0] = 1;
    arg.favmode = favmode;
    if (favmode)
        favlist[0] = 0;
    else
        favlist[0] = group;
    arg.namelist=NULL;
    while (1) {
        bzero((char *) &favboard_conf, sizeof(struct _select_def));
        arg.father = favlist[favlevel];
        if (favmode) {
            arg.newflag = 1;
            arg.yank_flag = BOARD_FAV;
        } else {
            arg.newflag = newflag;
            arg.yank_flag = yank_flag;
        }
        if (favmode) {
            getSession()->favnow = favlist[favlevel];
        } else {
            arg.boardprefix=boardprefix;
	 }
        arg.find = true;
        arg.loop_mode = 0;
		arg.select_group =0;
		if (favmode != 0 && lastloadfavmode != favmode){
 			load_favboard(1,favmode, getSession());
			lastloadfavmode = favmode;
		}
		arg.favmode = favmode;

        favboard_conf.item_per_page = BBS_PAGESIZE;
        favboard_conf.flag = LF_NUMSEL | LF_VSCROLL | LF_BELL | LF_LOOP | LF_MULTIPAGE;     /*|LF_HILIGHTSEL;*/
        favboard_conf.prompt = ">";
        favboard_conf.item_pos = pts;
        favboard_conf.arg = &arg;
        favboard_conf.title_pos.x = 0;
        favboard_conf.title_pos.y = 0;
        favboard_conf.pos = sellist[favlevel];
        favboard_conf.page_pos = ((sellist[favlevel]-1)/BBS_PAGESIZE)*BBS_PAGESIZE+1;
        if (arg.namelist) {
        	free(arg.namelist);
        	arg.namelist=NULL;
        }
        arg.reloaddata=false;

        if (favmode)        
            favboard_conf.get_data = fav_getdata;
        else
            favboard_conf.get_data = boards_getdata;
        (*favboard_conf.get_data)(&favboard_conf, favboard_conf.page_pos, BBS_PAGESIZE);
        if (favboard_conf.item_count==0) {
            if (arg.favmode || arg.yank_flag == BOARD_BOARDALL)
                break;
	    else {
                char ans[3];
                getdata(t_lines - 1, 0, "该讨论区组的版面已经被你全部取消了，是否查看所有讨论区？(Y/N)[N]", ans, 2, DOECHO, NULL, true);
                if (toupper(ans[0]) != 'Y')
                    break;
		arg.yank_flag=BOARD_BOARDALL;
                (*favboard_conf.get_data)(&favboard_conf, favboard_conf.page_pos, BBS_PAGESIZE);
                if (favboard_conf.item_count==0)
		    break;
	    }
        }
        fav_gotonextnew(&favboard_conf);
        favboard_conf.on_select = fav_onselect;
        favboard_conf.show_data = fav_show;
        favboard_conf.pre_key_command = fav_prekey;
        favboard_conf.key_command = fav_key;
        favboard_conf.show_title = fav_refresh;

        update_endline();
        if (list_select_loop(&favboard_conf) == SHOW_QUIT) {
            /*退出一层目录*/
            favlevel--;
            if (favlevel == -1)
                break;
            if (favlevel==changelevel){ //从版面目录返回收藏夹
                favmode=oldfavmode;
				if (favmode){
					selectlevel=-1;
				}
			}
        } else {
            /*选择了一个目录,SHOW_SELECT，注意有个假设，目录的深度
            不会大于FAVBOARDNUM，否则selist会溢出
            注意，arg->select_group被用来表示是select了版面还是用s跳转的。
            如果是s跳转，arg->select_group=true,否则arg->select_group=false;
            TODO: 动态增长sellist
            */
	/* stiger: select_group:  
	   在 select_loop退出，返回SHOW_SELECT的时候
	   select_group 0: 不是s进入目录
	                1: s进入版面目录
					<0, s进入allboard目录, i=0-select_group是 favboard_list[i-1]
					                       或者i=-1-select_group 是 favboard_list[i]
	*/
            sellist[favlevel] = favboard_conf.pos;

            if ((selectlevel==-1)||(arg.select_group==0))
                favlevel++;
	    	else
				favlevel=selectlevel; /*退回到∠一次的目录*/

			//原来在favboard
            if (favmode) {
				//进入版面目录
                if (arg.select_group > 0 || (arg.select_group==0 && (nbrd[favboard_conf.pos - favboard_conf.page_pos].flag!=0xffffffff) ) ) {
                    //s进入版面目录
                    if (arg.select_group > 0 ) //select进入的 
                        favlist[favlevel] = currboardent;
                    //非s进入版面目录
                    else
                        favlist[favlevel] = nbrd[favboard_conf.pos - favboard_conf.page_pos].pos+1;

                    changelevel=favlevel-1;
                    favmode=0;
				//进入收藏夹目录
                } else{
					//s进入的
					//进入公共收藏夹目录
					if (arg.select_group < 0){
                    	favlist[favlevel] = -1 - arg.select_group ;
						changelevel=favlevel-1;
						favmode=2;
					}else{
                        favlist[favlevel] = nbrd[favboard_conf.pos - favboard_conf.page_pos].tag;
					}
				}
            }
			//原来不在 favboard
            else {
                if (arg.select_group > 0) //select进入的
                    favlist[favlevel] = currboardent;
				else if (arg.select_group < 0){
                   	favlist[favlevel] = -1 - arg.select_group ;
					changelevel=favlevel-1;
					favmode=2;
				}else
                favlist[favlevel] = nbrd[favboard_conf.pos - favboard_conf.page_pos].pos+1;
            }
            if (arg.select_group != 0 ) //select进入的
		    selectlevel=favlevel;
            sellist[favlevel] = 1;
        };
        clear();
    }
    free(nbrd);
    free(pts);
    if (arg.namelist) {
    	free(arg.namelist);
    	arg.namelist=NULL;
    }
    save_zapbuf(getSession());
    modify_user_mode(oldmode);
    return 0;
}

extern int mybrd_list_t;

void FavBoard()
{
 	load_favboard(1,1, getSession());
    choose_board(1, NULL,0,1);
}

void AllBoard()
{
	load_favboard(1,2, getSession());
	choose_board(1, NULL,0,2);
}

void WwwBoard()
{
	load_favboard(1,3, getSession());
	choose_board(1, NULL,0,3);
}
