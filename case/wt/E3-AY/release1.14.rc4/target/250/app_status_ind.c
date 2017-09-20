#include "cmsis_os.h"
#include "stdbool.h"
#include "hal_trace.h"
#include "app_pwl.h"
#include "app_status_ind.h"
#include "string.h"

static APP_STATUS_INDICATION_T app_status = APP_STATUS_INDICATION_NUM;
static APP_STATUS_INDICATION_T app_status_ind_filter = APP_STATUS_INDICATION_NUM;

int app_status_indication_filter_set(APP_STATUS_INDICATION_T status)
{
    app_status_ind_filter = status;
    return 0;
}

APP_STATUS_INDICATION_T app_status_indication_get(void)
{
    return app_status;
}

//extern bool app_ledfresh;

int app_status_indication_set(APP_STATUS_INDICATION_T status)
{
    struct APP_PWL_CFG_T cfg0;
    struct APP_PWL_CFG_T cfg1;
   // TRACE("%s %d",__func__, status);

   // if(app_ledfresh) return 0;
	
    if (app_status == status)
        return 0;

    if (app_status_ind_filter == status)
        return 0;

    app_status = status;
    memset(&cfg0, 0, sizeof(struct APP_PWL_CFG_T));
    memset(&cfg1, 0, sizeof(struct APP_PWL_CFG_T));
    app_pwl_stop(APP_PWL_ID_0);
    app_pwl_stop(APP_PWL_ID_1);
    switch (status) {
        case APP_STATUS_INDICATION_POWERON:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (1500);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (10);

            cfg0.parttotal = 2;
            cfg0.startlevel = 1;
            cfg0.periodic = false;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);

            break;
        case APP_STATUS_INDICATION_INITIAL:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (200);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (200);
            cfg0.part[2].level = 1;
            cfg0.part[2].time = (200);
            cfg0.part[3].level = 0;
            cfg0.part[3].time = (200);
            cfg0.part[4].level = 1;
            cfg0.part[4].time = (200);
            cfg0.part[5].level = 0;
            cfg0.part[5].time = (200);
            cfg0.part[6].level = 1;
            cfg0.part[6].time = (200);

            cfg0.parttotal = 7;
            cfg0.startlevel = 1;
            cfg0.periodic = false;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);

            break;
			
		case APP_STATUS_INDICATION_INCOMINGCALL://来电
					//TRACE("来电");
					            cfg0.part[0].level = 1;
            cfg0.part[0].time = (200);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (1000);
            cfg0.parttotal = 2;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
					break;
							case APP_STATUS_INDICATION_PHONE://通话中
													cfg0.part[0].level = 1;
								cfg0.part[0].time = (200);
								cfg0.part[1].level = 0;
								cfg0.part[1].time = (6000);
								cfg0.parttotal = 2;
								cfg0.startlevel = 1;
								cfg0.periodic = true;
								app_pwl_setup(APP_PWL_ID_0, &cfg0);
								app_pwl_start(APP_PWL_ID_0);

					break;
		case APP_STATUS_INDICATION_OUTGOING://去电
								cfg0.part[0].level = 1;
			cfg0.part[0].time = (200);
			cfg0.part[1].level = 0;
			cfg0.part[1].time = (5000);
			cfg0.parttotal = 2;
			cfg0.startlevel = 1;
			cfg0.periodic = true;
			app_pwl_setup(APP_PWL_ID_0, &cfg0);
			app_pwl_start(APP_PWL_ID_0);
					break;	

		case APP_STATUS_INDICATION_MUS_PLAY://播放
			    //				TRACE("播放");
				//	break;
		case APP_STATUS_INDICATION_MUS_PAUSE://暂停
		//	TRACE("暂停");
			//break;

      //  case APP_STATUS_INDICATION_PAGESCAN://开机回连
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (100);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (5000);
            cfg0.parttotal = 2;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
			
			case APP_STATUS_INDICATION_RECNNT://开机回连
				cfg0.part[0].level = 1;
				cfg0.part[0].time = (200);
				cfg0.part[1].level = 0;
				cfg0.part[1].time = (200);
				cfg0.parttotal = 2;
				cfg0.startlevel = 1;
				cfg0.periodic = true;
				app_pwl_setup(APP_PWL_ID_0, &cfg0);
				app_pwl_start(APP_PWL_ID_0);
				break;


        case APP_STATUS_INDICATION_BOTHSCAN://配对  搜索            
            cfg0.part[0].level = 0;
            cfg0.part[0].time = (400);
            cfg0.part[1].level = 1;
            cfg0.part[1].time = (400);
            cfg0.parttotal = 2;
            cfg0.startlevel = 0;
            cfg0.periodic = true;

            cfg1.part[0].level = 1;
            cfg1.part[0].time = (400);
            cfg1.part[1].level = 0;
            cfg1.part[1].time = (400);
            cfg1.parttotal = 2;
            cfg1.startlevel = 1;
            cfg1.periodic = true;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;

        case APP_STATUS_INDICATION_CONNECTING:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (400);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (400);
            cfg0.parttotal = 2;
            cfg0.startlevel = 0;
            cfg0.periodic = true;

            cfg1.part[0].level = 1;
            cfg1.part[0].time = (400);
            cfg1.part[1].level = 0;
            cfg1.part[1].time = (400);
            cfg1.parttotal = 2;
            cfg1.startlevel = 1;
            cfg1.periodic = true;

            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_CONNECTED:
            cfg0.part[0].level = 1;
            cfg0.part[0].time = (500);
            cfg0.part[1].level = 0;
            cfg0.part[1].time = (3000);
            cfg0.parttotal = 2;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);
            break;
        case APP_STATUS_INDICATION_CHARGING:
            cfg1.part[0].level = 1;
            cfg1.part[0].time = (5000);
            cfg1.parttotal = 1;
            cfg1.startlevel = 1;
            cfg1.periodic = true;
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_FULLCHARGE:
           /* cfg0.part[0].level = 1;
            cfg0.part[0].time = (5000);
            cfg0.parttotal = 1;
            cfg0.startlevel = 1;
            cfg0.periodic = true;
            app_pwl_setup(APP_PWL_ID_0, &cfg0);
            app_pwl_start(APP_PWL_ID_0);*/
            break;
        case APP_STATUS_INDICATION_POWEROFF:
            cfg1.part[0].level = 1;
            cfg1.part[0].time = (200);
            cfg1.part[1].level = 0;
            cfg1.part[1].time = (200);
            cfg1.part[2].level = 1;
            cfg1.part[2].time = (200);
            cfg1.part[3].level = 0;
            cfg1.part[3].time = (200);
            cfg1.part[4].level = 1;
            cfg1.part[4].time = (200);
            cfg1.part[5].level = 0;
            cfg1.part[5].time = (200);
            cfg1.parttotal = 6;
            cfg1.startlevel = 1;
            cfg1.periodic = false;

            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);
            break;
        case APP_STATUS_INDICATION_CHARGENEED:
            cfg1.part[0].level = 1;
            cfg1.part[0].time = (500);
            cfg1.part[1].level = 0;
            cfg1.part[1].time = (2000);
            cfg1.parttotal = 2;
            cfg1.startlevel = 1;
            cfg1.periodic = true;
            app_pwl_setup(APP_PWL_ID_1, &cfg1);
            app_pwl_start(APP_PWL_ID_1);    
            break;
        default:
            break;
    }
    return 0;
}

